/*******************************************************************************
 *
 * $Id: parser.y 2891 2010-10-27 14:48:34Z dlp $
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   14.12.2007
 * -----
 *
 * Description:
 * ------------
 *
 * Parser of S-NET net interface.
 *
 *******************************************************************************/

%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "label.h"
#include "interface.h"
#include "memfun.h"
#include "snetentities.h"
#include "stream.h"
#include "constants.h"
#include "record_p.h"
#include "debug.h"

//LPEL
#include "task.h"
#include "stream.h"


#ifndef YY_BUF_SIZE
#define YY_BUF_SIZE 16384
#endif

/* SNet record types */
#define SNET_REC_DATA       "data" 
#define SNET_REC_COLLECT    "collect"
#define SNET_REC_SORT_BEGIN "sort_begin"
#define SNET_REC_SORT_END   "sort_end"
#define SNET_REC_TERMINATE  "terminate"
#define SNET_REC_TRIGGER    "trigger_initializer"

#define LABEL     "label"
#define INTERFACE "interface"
#define TYPE      "type"
#define MODE      "mode"
#define TEXTUAL   "textual"
#define BINARY    "binary"

#define MODE_BINARY 0
#define MODE_TEXTUAL 1
#define INTERFACE_UNKNOWN -1

 extern int yylex(void);
 extern void yylex_destroy();
 void yyerror(char *error);
 extern void yyrestart(FILE *);
 extern FILE *yyin;
 extern void yy_flush();
 
 /***** Struct definitions ******/

 /* Struct to represent xml attribute. */
 typedef struct attribute{
   char *name;
   char *value;
   struct attribute *next;
 } attrib_t;

 /***** Globals variables *****/
 static struct{

   /* Value that tells if a terminate record has been encounterd in the input stream */
   unsigned int terminate;  

   /* Labels to use with incoming fields/tags/btags */
   snetin_label_t *labels;

   /* Interfaces to use with incoming fields. */
   snetin_interface_t *interface;

   /* Stream where all the parsed data should be written to */
   stream_desc_t *output;
 }parser;

 /* Data values for record currently under parsing  */
 static struct{
   snet_record_t *record;
   int interface;
   int mode;
 }current;
 
 /** Attribute functions **
  *
  * These functions are used to handle XML-attributes in
  * the parser.
  *
  */

 /* Init XML-attribute names "name" with value "value" */
 static attrib_t *initAttribute(char *name, char *value){
   attrib_t *attrib = SNetMemAlloc(sizeof(attrib_t));

   attrib->name = name;

   attrib->value = value;

   attrib->next = NULL;
   return attrib;
 }

 /* Delete XML attribute */
 static void deleteAttribute(attrib_t *attrib){
   if(attrib != NULL){
     if(attrib->name != NULL){
       SNetMemFree(attrib->name);
     }
     if(attrib->value != NULL){
       SNetMemFree(attrib->value);
     }
     SNetMemFree(attrib);
   }
 }

 static void deleteAttributes(attrib_t *attrib){
   attrib_t *next = NULL;
  
   while(attrib != NULL) {
     next = attrib->next;
     deleteAttribute(attrib);
     attrib = next;
   }
 }

 /* Search XML-attribute list for attribute with given name
  * The first one that matches is returned  
  */
 static attrib_t *searchAttribute(attrib_t *ats, const char *name){
   if(name != NULL){
     attrib_t *temp = ats;
     while(temp != NULL){
       if(strcmp(temp->name, name) == 0){
	 return temp;
       }
       temp = temp->next;
     }
   }
   return NULL;
 }

%}

%union {
  char             *str;
  struct attribute *attrib;
}

%token STARTTAG_SHORTEND DQUOTE SQUOTE TAG_END RECORD_END_BEGIN FIELD_END_BEGIN
%token TAG_END_BEGIN BTAG_END_BEGIN RECORD_BEGIN FIELD_BEGIN TAG_BEGIN BTAG_BEGIN
%token EQ VERSION ENCODING STANDALONE YES NO VERSIONNUMBER XMLDECL_END XMLDECL_BEGIN
 
%token <str>  NAME CHARDATA DATTVAL SATTVAL ENCNAME 
%type <attrib> Attributes

%start Document

%%

Document:     Prolog Record
{
		/* YYACCEPT is needed to stop the parsing,
		   as there might be more characters in the
		   input stream! */
  
		YYACCEPT;
              }


/* At the moment these values are just ignored */ 
Prolog:       XMLDecl
              {

              }
            | /* EMPTY */
              {

              }
            ;


/* At the moment these values are just ignored */
XMLDecl:      XMLDECL_BEGIN VersionInfo EncodingDecl SDDecl XMLDECL_END
            | XMLDECL_BEGIN VersionInfo EncodingDecl XMLDECL_END 
            | XMLDECL_BEGIN VersionInfo SDDecl XMLDECL_END
            | XMLDECL_BEGIN VersionInfo XMLDECL_END
              {

              }
            ;

/* At the moment these values are just ignored */
VersionInfo:  VERSION EQ DQUOTE VERSIONNUMBER DQUOTE
            | VERSION EQ SQUOTE VERSIONNUMBER SQUOTE
              {

              }
            ;
/* At the moment these values are just ignored */
EncodingDecl: ENCODING EQ DQUOTE ENCNAME DQUOTE
              {
		if($4 != NULL){
		  SNetMemFree($4);
		}
              }
            | ENCODING EQ SQUOTE ENCNAME SQUOTE 
              {
		if($4 != NULL){
		  SNetMemFree($4);
		}
              }
            ;

/* At the moment these values are just ignored */
SDDecl:       STANDALONE EQ SQUOTE YES SQUOTE
            | STANDALONE EQ SQUOTE NO SQUOTE
            | STANDALONE EQ DQUOTE YES DQUOTE 
            | STANDALONE EQ DQUOTE NO DQUOTE
              {

              }
            ;

Record:       RECORD_BEGIN Attributes STARTTAG_SHORTEND
              {
		attrib_t *attr = searchAttribute($2, TYPE);
		if(attr != NULL) {

		  /* Data record: */
		  if(strcmp(attr->value, SNET_REC_DATA) == 0) {

		    attrib_t *interface = searchAttribute($2, INTERFACE);
		    attrib_t *mode = searchAttribute($2, MODE);

		    /* Default mode: */
		    current.mode = MODE_BINARY;

		    if(mode != NULL) {
		      if(strcmp(mode->value, TEXTUAL) == 0) {
			current.mode = MODE_TEXTUAL;
		      }
		      else if(strcmp(mode->value, BINARY) == 0) {
			current.mode = MODE_BINARY;
		      }
		    }
		  
		    if(interface != NULL) {
		      current.record = SNetRecCreate(REC_data,
					   SNetTencVariantEncode( 
					       SNetTencCreateVector( 0), 
					       SNetTencCreateVector( 0), 
					       SNetTencCreateVector( 0)));

		      current.interface = SNetInInterfaceToId(parser.interface, interface->value);
		      SNetRecSetInterfaceId(current.record, current.interface);

		      if(current.mode == MODE_BINARY) {
			SNetRecSetDataMode(current.record, MODE_binary);
		      }
		      else {
			SNetRecSetDataMode(current.record, MODE_textual);
		      }
		    }
		  }else if(strcmp(attr->value, SNET_REC_COLLECT) == 0){
		    SNetUtilDebugFatal("Input of REC_collect not implemented!");
		  }else if(strcmp(attr->value, SNET_REC_SORT_BEGIN) == 0){
		    SNetUtilDebugFatal("Input of REC_sort_begin not implemented!");
		  }else if(strcmp(attr->value, SNET_REC_SORT_END) == 0){
		    SNetUtilDebugFatal("Input of REC_sort_end not implemented!");
		  }else if(strcmp(attr->value, SNET_REC_TRIGGER) == 0){
		    SNetUtilDebugFatal("Input of REC_trigger_initializer not implemented!");
		  }else if(strcmp(attr->value, SNET_REC_TERMINATE) == 0){
		   
		    current.record = SNetRecCreate(REC_terminate);
		    if(parser.terminate != SNET_PARSE_ERROR){
		      parser.terminate = SNET_PARSE_TERMINATE;
		    }
		    
		  }else{
		    yyerror("Record with unknown type found!");
		  }
		}
		else{
		  yyerror("Record without type found!");
		}

		if(parser.terminate != SNET_PARSE_ERROR) {
		  if(current.record != NULL) {
                    /* write record to stream */
                    StreamWrite( parser.output, current.record);

		    current.record = NULL;
		    current.interface = INTERFACE_UNKNOWN;
		  }
		}else {
		  /* Discard the record because of parsing error. */
		  SNetRecDestroy(current.record);
		  current.record = NULL;
		  current.interface = INTERFACE_UNKNOWN;
		  yyerror("Error encountered while parsing a record. Record discarded (1)!");
		  parser.terminate = SNET_PARSE_CONTINUE;
		}

		deleteAttributes($2);
	      }

            | RECORD_BEGIN Attributes 
              { /* MID-RULE: */
		/* Form record and get default interface! */
		attrib_t *attr = searchAttribute($2, TYPE);
		if(attr != NULL) {
		  if(strcmp(attr->value, SNET_REC_DATA) == 0) {

		    attrib_t *interface = searchAttribute($2, INTERFACE);
		    attrib_t *mode = searchAttribute($2, MODE);

		    current.record = SNetRecCreate(REC_data,
					 SNetTencVariantEncode( 
					     SNetTencCreateVector( 0), 
					     SNetTencCreateVector( 0), 
					     SNetTencCreateVector( 0)));


		    /* Default mode: */
		    current.mode = MODE_BINARY;

		    if(mode != NULL) {
		      if(strcmp(mode->value, TEXTUAL) == 0) {
			current.mode = MODE_TEXTUAL;
		      }
		      else if(strcmp(mode->value, BINARY) == 0) {
			current.mode = MODE_BINARY;
		      }
		    }
	
		    if(current.mode == MODE_BINARY) {
		      SNetRecSetDataMode(current.record, MODE_binary);
		    }
		    else {
		      SNetRecSetDataMode(current.record, MODE_textual);
		    }
		    
		    if(interface != NULL) {
		      current.interface = SNetInInterfaceToId(parser.interface, 
							      interface->value);
		      SNetRecSetInterfaceId(current.record, current.interface);
		    }else {
		      SNetRecSetInterfaceId(current.record, INTERFACE_UNKNOWN);
		    }

		  }else if(strcmp(attr->value, SNET_REC_COLLECT) == 0){
		    SNetUtilDebugFatal("Input of REC_collect not implemented!");
		  }else if(strcmp(attr->value, SNET_REC_SORT_BEGIN) == 0){
		    SNetUtilDebugFatal("Input of REC_sort_begin not implemented!");
		  }else if(strcmp(attr->value, SNET_REC_SORT_END) == 0){
		    SNetUtilDebugFatal("Input of REC_sort_end not implemented!");
		  }else if(strcmp(attr->value, SNET_REC_TRIGGER) == 0){
		    SNetUtilDebugFatal("Input of REC_trigger_initializer not implemented!");
		  }else if(strcmp(attr->value, SNET_REC_TERMINATE) == 0){
		    /* New control record: terminate */
		    current.record = SNetRecCreate(REC_terminate);
		    if(parser.terminate != SNET_PARSE_ERROR){
		      parser.terminate = SNET_PARSE_TERMINATE;
		    }
		  }else{
		    yyerror("Record with unknown type found!");
		  }
		}else{
		    yyerror("Record without type found!");
		}
	      } 
              TAG_END Entitys RECORD_END_BEGIN TAG_END{
		if(current.record != NULL) {

		  if(parser.terminate != SNET_PARSE_ERROR) {
		    if(SNetRecGetDescriptor(current.record) == REC_data
		       && SNetRecGetInterfaceId(current.record) == INTERFACE_UNKNOWN) {
		      SNetRecDestroy(current.record);
		      current.record = NULL;
		      current.interface = INTERFACE_UNKNOWN;
		      yyerror("Error encountered while parsing a record. Record discarded (2)!");
		      parser.terminate = SNET_PARSE_CONTINUE;
		    } else {
                      /* write record to stream */
                      StreamWrite( parser.output, current.record);

		      current.record = NULL;
		      current.interface = INTERFACE_UNKNOWN;
		    }
		  }else {
		    /* Discard the record because of parsing error. */
		    SNetRecDestroy(current.record);
		    current.record = NULL;
		    current.interface = INTERFACE_UNKNOWN;
		    yyerror("Error encountered while parsing a record. Record discarded (3)!");
		    parser.terminate = SNET_PARSE_CONTINUE;
		  }
		  
		  current.record = NULL;
		}

		deleteAttributes($2);

              }
          ;

Entitys:  Field Entitys
          {

	  }
        | Tag Entitys
          {

	  }
        | Btag Entitys
          {

	  }
        | /* EMPTY */
          {
	  }
        ;


Field:    FIELD_BEGIN Attributes STARTTAG_SHORTEND
          {
	    /* Field without any data. */ 
	    SNetUtilDebugFatal("Input: Field without data encountered!");
	  }
        | FIELD_BEGIN Attributes TAG_END
          { /* MID-RULE: */

	    attrib_t *attr = NULL;
	    void *data = NULL;
	    int iid;
	    attrib_t *finterface;
	    int label;
  
	    attr = searchAttribute($2, LABEL);
	    if(attr != NULL) {
	      label = SNetInLabelToId(parser.labels, attr->value);
	    } else{
	      yyerror("Field without label found!");
	    }

	    finterface = searchAttribute($2, INTERFACE);

	    if(finterface == NULL) {
	      /* This can be INTERFACE_UNKNOWN!*/
	      iid = current.interface;
	    }else {
	      int i = SNetInInterfaceToId(parser.interface, finterface->value);
	      
	      if(current.interface != INTERFACE_UNKNOWN) {
		if(i != current.interface) {
		  /* The record and the field have different interfaces! */
		  yyerror("Interface error!");
		  i = INTERFACE_UNKNOWN;
		}
	      }else {
		if(i == SNET_INTERFACE_ERROR) {
		  /* The field has unknown interface! */
		  yyerror("Interface error!");
		  i = INTERFACE_UNKNOWN;
		}else {
		  /* No default interface declared. The record might have
		   * interface if this is not the first field. */
		  iid = SNetRecGetInterfaceId(current.record);

		  if(iid == INTERFACE_UNKNOWN) {
		    /* No interface set yet. This field dictates the interface. */
		    SNetRecSetInterfaceId(current.record, i);
		    iid = i;
		  } else if(iid != i) {
		    /* The record and the field have different interfaces! */
		    yyerror("Interface error!");
		    i = INTERFACE_UNKNOWN;
		  }
		}
	      }
	      
	      iid = i;
	    }
	
	    if(iid != INTERFACE_UNKNOWN) {
	
        if(current.mode == MODE_TEXTUAL) {
          data = SNetGetDeserializationFun(iid)(yyin);
	      } else if(current.mode == MODE_BINARY) {
		        data = SNetGetDecodingFun(iid)(yyin);
	      } else {
		        data = NULL;
		        yyerror("Unknown data mode");
	      }

	      if(data != NULL) {
      		SNetRecAddField(current.record, label);
	      	SNetRecSetField(current.record, label, data);
	      } else {
		      yyerror("Could not decode data!");
	      }
	     
	      while(getc(yyin) != '<');		  
	      if(ungetc('<', yyin) == EOF){
		/* This is an error. First char of the next tag is already consumed! */
      		SNetUtilDebugFatal("Input: Reading error.");
	      }
	      yyrestart(yyin);
	    }else { 
	      /* If we cannot deserialise the data we must ignore it! */
	     
	      while(getc(yyin) != '<');
	      if(ungetc('<', yyin) == EOF){
		/* This is an error. First char of the next tag is already consumed! */
		SNetUtilDebugFatal("Input: Reading error.");
	      }
	    }
          }
          FIELD_END_BEGIN TAG_END
          {
	  
	    deleteAttributes($2);
          }
        ;

Tag:      TAG_BEGIN Attributes STARTTAG_SHORTEND
          {
	    /* Tag with no value. */
            attrib_t *attr = NULL;
	    int label;
	    
	    attr = searchAttribute($2, LABEL);
	    if(attr != NULL) {
	      label = SNetInLabelToId(parser.labels, attr->value);
	    
	      SNetRecAddTag(current.record, label);

	      SNetRecSetTag(current.record, label, 0);

	    } else{
	      yyerror("Tag without label found!");
	    }


	    deleteAttributes($2);
	  }
        | TAG_BEGIN Attributes TAG_END CHARDATA TAG_END_BEGIN TAG_END
          {
	    attrib_t *attr = NULL;
	    int label;
	    
	    attr = searchAttribute($2, LABEL);
	    if(attr != NULL) {
	      label = SNetInLabelToId(parser.labels, attr->value);
	    
	      SNetRecAddTag(current.record, label);

	      /* TODO: test that the atoi call worked? */
	      SNetRecSetTag(current.record, label, atoi($4));

	      SNetMemFree($4);
	    } else{
	      yyerror("Tag without label found!");
	    }


	    deleteAttributes($2);
          }
        | TAG_BEGIN Attributes TAG_END TAG_END_BEGIN TAG_END
          {
	    attrib_t *attr = NULL;
	    int label;
	    
	    attr = searchAttribute($2, LABEL);
	    if(attr != NULL) {
	      label = SNetInLabelToId(parser.labels, attr->value);
	    
	      SNetRecAddTag(current.record, label);

	      SNetRecSetTag(current.record, label, 0);
	    } else{
	      yyerror("Tag without label found!");
	    }


	    deleteAttributes($2);
          }
        ;

Btag:     BTAG_BEGIN Attributes STARTTAG_SHORTEND
          {
	    /* Btag with no value. */

	    attrib_t *attr = NULL;
	    int label;
	    
	    attr = searchAttribute($2, LABEL);
	    if(attr != NULL) {
	      label = SNetInLabelToId(parser.labels, attr->value);
	    
	      SNetRecAddBTag(current.record, label);
	      
	      SNetRecSetBTag(current.record, label, 0);
	      
	    } else{
	      yyerror("Btag without label found!");
	    }
	    
	    deleteAttributes($2);
	  }
        | BTAG_BEGIN Attributes TAG_END CHARDATA BTAG_END_BEGIN TAG_END
          {
	    attrib_t *attr = NULL;
	    int label;
	    
	    attr = searchAttribute($2, LABEL);
	    if(attr != NULL) {
	      label = SNetInLabelToId(parser.labels, attr->value);
	    
	      SNetRecAddBTag(current.record, label);

	      /* TODO: test that the atoi call worked correctly? */
	      SNetRecSetBTag(current.record, label, atoi($4));

	      SNetMemFree($4);
	    } else{
	      yyerror("Btag without label found!");
	    }

	    deleteAttributes($2);
          }
        | BTAG_BEGIN Attributes TAG_END BTAG_END_BEGIN TAG_END
          {
	    attrib_t *attr = NULL;
	    int label;
	    
	    attr = searchAttribute($2, LABEL);
	    if(attr != NULL) {
	      label = SNetInLabelToId(parser.labels, attr->value);
	    
	      SNetRecAddBTag(current.record, label);

	      SNetRecSetBTag(current.record, label, 0);

	    } else{
	      yyerror("Btag without label found!");
	    }

	    deleteAttributes($2);
          }
        ;

Attributes:   NAME EQ SQUOTE SATTVAL SQUOTE Attributes 
              {  
         
		/* Default namespace */
		if(strcmp($1, "xmlns") == 0){
		  if(strcmp($4, SNET_NAMESPACE) != 0) {
		    // Do we even want to check this?
		    SNetUtilDebugFatal("Input: Reading error: Incorrect namespace.");
		  }
		  SNetMemFree($4);
		  SNetMemFree($1);
		  $$ = $6;
		}/* Normal attribute */
		else{
		  attrib_t *temp = initAttribute($1, $4);
		  temp->next = $6;
		  $$ = temp;
		}
              }

            | NAME EQ DQUOTE DATTVAL DQUOTE Attributes 
              {  
		/* Default namespace */
		if(strcmp($1, "xmlns") == 0){
		  if(strcmp($4, SNET_NAMESPACE) != 0) {
		    // Do we even want to check this?
		    SNetUtilDebugFatal("Input: Reading error: Incorrect namespace.");
		  }
		  SNetMemFree($4);
		  SNetMemFree($1);
		  $$ = $6;
		}/* Normal attribute */
		else{
		  attrib_t *temp = initAttribute($1, $4);
		  temp->next = $6;;
		  $$ = temp;
		}
              }

            | /* EMPTY */
              {                
                $$ = NULL;
              }
%%

static void parserflush()
{
  if(current.record != NULL) {
    SNetRecDestroy(current.record);
    current.record = NULL;
  }

  current.interface = INTERFACE_UNKNOWN;

  current.mode = MODE_BINARY;

  if(parser.terminate == SNET_PARSE_ERROR) {
    parser.terminate = SNET_PARSE_CONTINUE;
  }

  yy_flush();
}

void yyerror(char *error) 
{
  if(error != NULL && strcmp(error, "syntax error") != 0) {
    printf("\n  ** Parse error: %s\n\n", error);
  }

  if(parser.terminate != SNET_PARSE_TERMINATE) {
    parser.terminate = SNET_PARSE_ERROR;
  }
}

void SNetInParserInit(FILE *file,
		      snetin_label_t *labels,
		      snetin_interface_t *interfaces,
                      stream_desc_t *output)
{  
  yyin = file; 
  parser.labels = labels;
  parser.interface = interfaces;
  parser.output = output;

  parser.terminate = SNET_PARSE_CONTINUE;
}  

int SNetInParserParse()
{
  parserflush();

  if(parser.terminate == SNET_PARSE_CONTINUE) {
    yyparse();
  }

  return parser.terminate;
}

void SNetInParserDestroy()
{
  yylex_destroy();
}

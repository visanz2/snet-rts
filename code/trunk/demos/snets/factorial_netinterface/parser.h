#ifndef PARSER_H_
#define PARSER_H_

#include <snetentities.h>
#include "label.h"
#include "interface.h"

/* Return values of parserParse() */

#define PARSE_CONTINUE 0  /* Parsing can continue after this. */
#define PARSE_TERMINATE 1 /* Parsing should terminate after this. */
#define PARSE_ERROR 2     /* Error while parsing the last input data. */

/* Initialize the parse before parsing.
 *
 * @param in_buf Buffer where the parsed records are put.
 * @param label  Label data to use while parsing.
 * @param interfaces Interface mapping to be used for language interfaces.
 * @param fdeserialize Deserialization function for parsed data values.
 * @param ffree Free function for parsed data values.
 * @param fcopy Copy function for the parsed data values.
 *
 * @notice parserInit should be the first call to the parser!
 */

extern void parserInit(snet_buffer_t *in_buf,
		       label_t *label,
		       interface_t *interfaces);


/* Flush all data in parser. 
 */

extern void parserFlush();


/* Parse the next data element from standard input stream 
 *
 * @return PARSE_CONTINUE Parsing can continue after this.
 * @return PARSE_TERMINATE Parsing should terminate after this.
 * @return PARSE_ERROR Error while parsing the last input data.
 *
 * @notice parserInit() MUST be called before the first call to parserParse()! 
 */

extern int parserParse();


/* Delete the parser and all data stored by it.
 * 
 * @notice Calling any parser functions after parserDelete() call might lead 
 *         to unexpected results!
 */

extern void parserDelete();

#endif /* PR_H_ */

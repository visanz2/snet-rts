/**
 * @file ts01.c
 *
 * Source code of compiled snet-file for runtime.
 *
 * THIS FILE HAS BEEN GENERATED.
 * DO NOT EDIT THIS FILE.
 * EDIT THE ORIGINAL SNET-SOURCE FILE ts01.snet INSTEAD!
 *
 * ALL CHANGES MADE TO THIS FILE WILL BE OVERWRITTEN!
 *
*/

#include "ts01.snet.h"
#include "networkinterface.h"

char *snet_ts01_labels[] = {
	"E__ts01__None",
	"a",
	"b",
	"c",
	"d",
	"e",
	"f",
	"T"};

char *snet_ts01_interfaces[] = {};


static void SNet__ts01__A(snet_handle_t *hnd)
{
  snet_record_t *rec = NULL;
  void *field_a = NULL;

  rec = SNetHndGetRecord(hnd);

  field_a = SNetRecTakeField(rec, F__ts01__a);

  SNetCall__A(hnd, field_a);
}


static void SNet__ts01__B(snet_handle_t *hnd)
{
  snet_record_t *rec = NULL;
  void *field_b = NULL;

  rec = SNetHndGetRecord(hnd);

  field_b = SNetRecTakeField(rec, F__ts01__b);

  SNetCall__B(hnd, field_b);
}


static void SNet__ts01__C(snet_handle_t *hnd)
{
  snet_record_t *rec = NULL;
  void *field_c = NULL;

  rec = SNetHndGetRecord(hnd);

  field_c = SNetRecTakeField(rec, F__ts01__c);

  SNetCall__C(hnd, field_c);
}


static void SNet__ts01__D(snet_handle_t *hnd)
{
  snet_record_t *rec = NULL;
  void *field_d = NULL;

  rec = SNetHndGetRecord(hnd);

  field_d = SNetRecTakeField(rec, F__ts01__d);

  SNetCall__D(hnd, field_d);
}


static void SNet__ts01__E(snet_handle_t *hnd)
{
  snet_record_t *rec = NULL;
  void *field_e = NULL;

  rec = SNetHndGetRecord(hnd);

  field_e = SNetRecTakeField(rec, F__ts01__e);

  SNetCall__E(hnd, field_e);
}

static snet_tl_stream_t *SNet__ts01___SL___SL(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;
  snet_typeencoding_t *out_type = NULL;
  snet_box_sign_t *out_sign = NULL;

  out_type = SNetTencTypeEncode(2, 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__ts01__b), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__ts01__b), 
                SNetTencCreateVector(1, T__ts01__T), 
                SNetTencCreateVector(0)));


  out_sign = SNetTencBoxSignEncode( out_type, 
              SNetTencCreateVector(1, field), 
              SNetTencCreateVector(2, field, tag));


  out_buf = SNetBox(in_buf, "A", 
              &SNet__ts01__A, 
              out_sign);

  return (out_buf);
}

static snet_tl_stream_t *SNet__ts01___SL___SR___ST___SL(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;
  snet_typeencoding_t *out_type = NULL;
  snet_box_sign_t *out_sign = NULL;

  out_type = SNetTencTypeEncode(2, 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__ts01__c), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__ts01__d), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)));


  out_sign = SNetTencBoxSignEncode( out_type, 
              SNetTencCreateVector(1, field), 
              SNetTencCreateVector(1, field));


  out_buf = SNetBox(in_buf, "B", 
              &SNet__ts01__B, 
              out_sign);

  return (out_buf);
}

static snet_tl_stream_t *SNet__ts01___SL___SR___ST___SR___P1(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;
  snet_typeencoding_t *out_type = NULL;
  snet_box_sign_t *out_sign = NULL;

  out_type = SNetTencTypeEncode(3, 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__ts01__b), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__ts01__b), 
                SNetTencCreateVector(1, T__ts01__T), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(2, F__ts01__b, F__ts01__e), 
                SNetTencCreateVector(1, T__ts01__T), 
                SNetTencCreateVector(0)));


  out_sign = SNetTencBoxSignEncode( out_type, 
              SNetTencCreateVector(1, field), 
              SNetTencCreateVector(2, field, tag), 
              SNetTencCreateVector(3, field, field, tag));


  out_buf = SNetBox(in_buf, "C", 
              &SNet__ts01__C, 
              out_sign);

  return (out_buf);
}

static snet_tl_stream_t *SNet__ts01___SL___SR___ST___SR___P2(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;
  snet_typeencoding_t *out_type = NULL;
  snet_box_sign_t *out_sign = NULL;

  out_type = SNetTencTypeEncode(3, 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__ts01__b), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__ts01__b), 
                SNetTencCreateVector(1, T__ts01__T), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(2, F__ts01__b, F__ts01__e), 
                SNetTencCreateVector(1, T__ts01__T), 
                SNetTencCreateVector(0)));


  out_sign = SNetTencBoxSignEncode( out_type, 
              SNetTencCreateVector(1, field), 
              SNetTencCreateVector(2, field, tag), 
              SNetTencCreateVector(3, field, field, tag));


  out_buf = SNetBox(in_buf, "D", 
              &SNet__ts01__D, 
              out_sign);

  return (out_buf);
}

static snet_tl_stream_t *SNet__ts01___SL___SR___ST___SR(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetParallelDet(in_buf, 
              SNetTencCreateTypeEncodingList(2, 
                SNetTencTypeEncode(1, 
                  SNetTencVariantEncode(
                    SNetTencCreateVector(1, F__ts01__c), 
                    SNetTencCreateVector(0), 
                    SNetTencCreateVector(0))), 
                SNetTencTypeEncode(1, 
                  SNetTencVariantEncode(
                    SNetTencCreateVector(1, F__ts01__d), 
                    SNetTencCreateVector(0), 
                    SNetTencCreateVector(0)))), 
              &SNet__ts01___SL___SR___ST___SR___P1, 
              &SNet__ts01___SL___SR___ST___SR___P2);

  return (out_buf);
}

static snet_tl_stream_t *SNet__ts01___SL___SR___ST(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetSerial(in_buf, 
              &SNet__ts01___SL___SR___ST___SL, 
              &SNet__ts01___SL___SR___ST___SR);

  return (out_buf);
}

static snet_tl_stream_t *SNet____STAR_INCARNATE_ts01___SL___SR(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetStarIncarnate(in_buf, 
              SNetTencTypeEncode(1, 
                SNetTencVariantEncode(
                  SNetTencCreateVector(1, F__ts01__e), 
                  SNetTencCreateVector(1, T__ts01__T), 
                  SNetTencCreateVector(0))), 
              SNetEcreateList(1, 
                SNetElt( 
                  SNetEtag( T__ts01__T), 
                  SNetEconsti( 5))), 
              &SNet__ts01___SL___SR___ST, 
              &SNet____STAR_INCARNATE_ts01___SL___SR);

  return (out_buf);
}

static snet_tl_stream_t *SNet__ts01___SL___SR(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetStar(in_buf, 
              SNetTencTypeEncode(1, 
                SNetTencVariantEncode(
                  SNetTencCreateVector(1, F__ts01__e), 
                  SNetTencCreateVector(1, T__ts01__T), 
                  SNetTencCreateVector(0))), 
              SNetEcreateList(1, 
                SNetElt( 
                  SNetEtag( T__ts01__T), 
                  SNetEconsti( 5))), 
              &SNet__ts01___SL___SR___ST, 
              &SNet____STAR_INCARNATE_ts01___SL___SR);

  return (out_buf);
}

static snet_tl_stream_t *SNet__ts01___SL(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetSerial(in_buf, 
              &SNet__ts01___SL___SL, 
              &SNet__ts01___SL___SR);

  return (out_buf);
}

static snet_tl_stream_t *SNet__ts01___SR___IS(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;
  snet_typeencoding_t *out_type = NULL;
  snet_box_sign_t *out_sign = NULL;

  out_type = SNetTencTypeEncode(2, 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__ts01__e), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)), 
              SNetTencVariantEncode(
                SNetTencCreateVector(1, F__ts01__f), 
                SNetTencCreateVector(0), 
                SNetTencCreateVector(0)));


  out_sign = SNetTencBoxSignEncode( out_type, 
              SNetTencCreateVector(1, field), 
              SNetTencCreateVector(1, field));


  out_buf = SNetBox(in_buf, "E", 
              &SNet__ts01__E, 
              out_sign);

  return (out_buf);
}

static snet_tl_stream_t *SNet__ts01___SR(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetSplit(in_buf, 
              &SNet__ts01___SR___IS, T__ts01__T, T__ts01__T);

  return (out_buf);
}

snet_tl_stream_t *SNet__ts01___ts01(snet_tl_stream_t *in_buf)
{
  snet_tl_stream_t *out_buf = NULL;

  out_buf = SNetSerial(in_buf, 
              &SNet__ts01___SL, 
              &SNet__ts01___SR);

  return (out_buf);
}

int SNetMain__ts01(int argc, char* argv[])
{
  int ret = 0;

  SNetGlobalInitialise();

  ret = SNetInRun(argc, argv,
              snet_ts01_labels,
              SNET__ts01__NUMBER_OF_LABELS,
              snet_ts01_interfaces,
              SNET__ts01__NUMBER_OF_INTERFACES,
              SNet__ts01___ts01);

  SNetGlobalDestroy();

  return( ret);
}


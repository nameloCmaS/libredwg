#define ENCODE_TEST_C
#define _DEFAULT_SOURCE 1
#if defined(__linux__)
#  define _GNU_SOURCE 1 /* for memmem on linux */
#endif
#define _BSD_SOURCE 1
#ifdef __STDC_ALLOC_LIB__
#  define __STDC_WANT_LIB_EXT2__ 1 /* for strdup */
#else
#  define _USE_BSD 1
#endif

extern unsigned int loglevel;

#define IS_ENCODER
#include <stdlib.h>
// #include "common.h"
// CLANG_DIAG_IGNORE (-Wpragma-pack)
#include "encode.c"
#include "common.c"
// CLANG_DIAG_RESTORE
#include "decode.h"
#include "tests_common.h"

// from in_json.h
extern int dwg_read_json (Bit_Chain *restrict dat, Dwg_Data *restrict dwg);
// from in_dxf.h
extern int dwg_read_dxf (Bit_Chain *restrict dat, Dwg_Data *restrict dwg);
// from out_json.h
extern int dwg_write_json_dat (Bit_Chain *restrict dat,
                               Dwg_Data *restrict dwg);
// from dwg_api.h
extern int dwg_add_Document (Dwg_Data *restrict dwg, const int imperial);
extern int dwg_add_class (Dwg_Data *restrict dwg,
                          const char *const restrict dxfname,
                          const char *const restrict cppname,
                          const char *const restrict appname,
                          const bool is_entity);
extern Dwg_Entity_REGION *
dwg_add_REGION (Dwg_Object_BLOCK_HEADER *restrict blkhdr,
                const char *acis_data);

BITCODE_RL size = SECTION_R13_SIZE;
// Dwg_Section_Type_r13 section_order[SECTION_R13_SIZE] = { 0 };

static int
find_duplicates (const Dwg_Data *dwg)
{
  for (Dwg_Section_Type_r13 i = 0; (unsigned)i < size; i++)
    {
      Dwg_Section_Type_r13 id = section_order[i];
      for (Dwg_Section_Type_r13 j = 0; (unsigned)j < size; j++)
        {
          if (i == j)
            continue;
          if (section_order[j] == id && (unsigned)id != size)
            {
              fail ("find_duplicate %u", (unsigned)id);
              section_order_trace (dwg, size,
                                   (Dwg_Section_Type_r13 *)&section_order);
              return 1;
            }
        }
    }
  return 0;
}

static void
section_reset (const Dwg_Data *dwg)
{
  memset (section_order, 0, sizeof section_order);
  for (Dwg_Section_Type_r13 i = 0; (unsigned)i < size; i++)
    section_order[i] = i;
  fprintf (stderr, "---\n");
  section_order_trace (dwg, size, (Dwg_Section_Type_r13 *)&section_order);
}

static void
test_section_find (const Dwg_Data *dwg)
{
  unsigned id;
  size = SECTION_R13_SIZE;
  section_reset (dwg);
  for (Dwg_Section_Type_r13 i = 0; (unsigned)i <= size; i++)
    {
      id = section_find ((Dwg_Section_Type_r13 *)&section_order, size, i);
      if (id != (unsigned)i) // 7 not found, returns SECTION_R13_SIZE
        {
          fail ("section_find %u => %u", (unsigned)i, (unsigned)id);
          section_order_trace (dwg, size,
                               (Dwg_Section_Type_r13 *)&section_order);
        }
    }
  id = section_find ((Dwg_Section_Type_r13 *)&section_order, size, size + 1);
  if (id != SECTION_R13_SIZE) // not found
    {
      fail ("section_find %u => %u", size + 1, id);
      section_order_trace (dwg, size, (Dwg_Section_Type_r13 *)&section_order);
    }
}

static void
test_section_move_top (const Dwg_Data *dwg)
{
  int err = 0;
  size = SECTION_R13_SIZE - 1;
  section_reset (dwg);
  for (Dwg_Section_Type_r13 i = 0; (unsigned)i < size - 1; i++)
    {
      // without insert
      if (section_move_top ((Dwg_Section_Type_r13 *)&section_order, &size, i))
        {
          err++;
          fail ("section_move_top existing %u", (unsigned)i);
        }
      section_order_trace (dwg, size, (Dwg_Section_Type_r13 *)&section_order);
      if (section_order[0] != i)
        {
          err++;
          fail ("section_move_top %u not first", (unsigned)i);
        }
      if (find_duplicates (dwg))
        {
          err++;
          fail ("duplicates");
        }
    }
  // with insert
  if (!section_move_top ((Dwg_Section_Type_r13 *)&section_order, &size, size))
    {
      err++;
      fail ("section_move_top insert");
    }
  section_order_trace (dwg, size, (Dwg_Section_Type_r13 *)&section_order);
  if (!err)
    ok ("test_section_move_top");
  else
    fail ("test_section_move_top");
  size = SECTION_R13_SIZE;
}

static inline unsigned
maxrand (unsigned max)
{
  unsigned rnd = (unsigned)rand ();
  return rnd % max;
}

static void
test_section_remove (const Dwg_Data *dwg)
{
  int err = 0;
  int sz = size - 1;
  section_reset (dwg);
  for (unsigned i = 0; i < size; i++)
    {
      Dwg_Section_Type_r13 id = maxrand (size);
      if (section_remove ((Dwg_Section_Type_r13 *)&section_order, &size, id))
        sz--;
      section_order_trace (dwg, size, (Dwg_Section_Type_r13 *)&section_order);
      if (find_duplicates (dwg))
        {
          err++;
          fail ("test_section_remove");
        }
    }
  if (!err)
    ok ("test_section_remove");
  else
    fail ("test_section_remove");
}

static void
test_section_move_before (const Dwg_Data *dwg)
{
  int err = 0;
  size = SECTION_R13_SIZE - 2;
  section_reset (dwg);
  for (unsigned i = 0; i < size; i++)
    {
      Dwg_Section_Type_r13 id = maxrand (size);
      Dwg_Section_Type_r13 before = maxrand (size);
      while (before == id)
        before = maxrand (size);
      if (section_move_before ((Dwg_Section_Type_r13 *)&section_order, &size,
                               id, before))
        {
          fail ("move_before %u", (unsigned)id);
          err++;
        }
      section_order_trace (dwg, size, (Dwg_Section_Type_r13 *)&section_order);
      err += find_duplicates (dwg);
    }
  if (!section_move_before ((Dwg_Section_Type_r13 *)&section_order, &size, 6,
                            4))
    {
      fail ("move_before %u inserts", 6);
      err++;
    }
  err += find_duplicates (dwg);
  if (!err)
    ok ("test_section_move_before");
  else
    fail ("test_section_move_before");
  size = SECTION_R13_SIZE;
}

static void
compress_R2004_section_tests (void)
{
  int result;
  uint32_t comp_data_size;
  Bit_Chain comp = { 0 }, dec = { 0 };
  /* from decode_test.c: decompressed AuxHeader data (123 bytes) */
  unsigned char decomp_auxh_bin[123]
      = { 0xff, 0x88, 0x01, 0x21, 0x00, 0x1d, 0x00, 0x19, 0x00, 0x00, 0x00,
          0xff, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x16, 0x00, 0x2e, 0x00, 0x16, 0x00, 0x2e, 0x00, 0x04, 0x00,
          0x65, 0x05, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x07, 0x00, 0xea, 0x74, 0x25,
          0x00, 0x9a, 0xe6, 0x33, 0x04, 0xb0, 0x82, 0x25, 0x00, 0xe0, 0x1c,
          0xf7, 0x01, 0xe9, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x0d, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00 };
  unsigned char decomp_ofs_bin[53]
      = { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xb0, 0x82, 0x25,
          0x00, 0xe0, 0x1c, 0xf7, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0x32,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 };

  /* Test 1: compress auxh, decompress, compare */
  bit_chain_alloc_size (&comp, sizeof decomp_auxh_bin * 2);
  comp.byte = 0;
  comp.bit = 0;
  result = compress_R2004_section (&comp, decomp_auxh_bin,
                                   sizeof decomp_auxh_bin, &comp_data_size);
  if (result)
    {
      fail ("compress_R2004_section auxh returned %d", result);
      goto test2;
    }
  /* decompress and verify roundtrip */
  comp.byte = 0;
  comp.bit = 0;
  comp.size = comp_data_size;
  bit_chain_alloc_size (&dec, sizeof decomp_auxh_bin);
  dec.size = sizeof decomp_auxh_bin;
  dec.byte = 0;
  dec.bit = 0;
  result = decompress_R2004_section (&comp, &dec);
  if (result == 0 && dec.size == sizeof decomp_auxh_bin
      && memcmp (dec.chain, decomp_auxh_bin, sizeof decomp_auxh_bin) == 0)
    pass ();
  else
    fail ("compress_R2004_section auxh roundtrip %d %lu", result,
          (unsigned long)dec.size);

test2:
  /* Test 2: compress ofs, decompress, compare */
  comp.byte = 0;
  comp.bit = 0;
  if (comp.size < sizeof decomp_ofs_bin * 2)
    bit_chain_alloc_size (&comp, sizeof decomp_ofs_bin * 2);
  result = compress_R2004_section (&comp, decomp_ofs_bin,
                                   sizeof decomp_ofs_bin, &comp_data_size);
  if (result)
    {
      fail ("compress_R2004_section ofs returned %d", result);
      goto cleanup;
    }
  comp.byte = 0;
  comp.bit = 0;
  comp.size = comp_data_size;
  dec.byte = 0;
  dec.bit = 0;
  dec.size = sizeof decomp_ofs_bin;
  result = decompress_R2004_section (&comp, &dec);
  if (result == 0
      && memcmp (dec.chain, decomp_ofs_bin, sizeof decomp_ofs_bin) == 0)
    ok ("compress_R2004_section");
  else
    fail ("compress_R2004_section ofs roundtrip %d %lu", result,
          (unsigned long)dec.size);

cleanup:
  free (comp.chain);
  free (dec.chain);
}

/* Regression test for double-free fix in COMMON_ENTITY_HANDLE_DATA
   (commit 6deac10e).  The macro copies *hdl_dat to dat1 then NULLs
   hdl_dat->chain so dat1 owns the buffer exclusively.  Without the
   NULL, bit_chain_free(hdl_dat) after bit_chain_free(&dat1) would
   double-free. */
static void
common_entity_handle_data_double_free_test (void)
{
  Bit_Chain hdl_dat = { 0 };
  char *buf = (char *)calloc (64, 1);
  if (!buf)
    {
      fail ("common_entity_handle_data: calloc failed");
      return;
    }
  hdl_dat.chain = (unsigned char *)buf;
  hdl_dat.size = 64;
  hdl_dat.byte = 0;
  hdl_dat.bit = 0;

  /* Simulate the macro's copy-then-NULL pattern */
  {
    Bit_Chain dat1 = hdl_dat; /* copy: dat1.chain = hdl_dat.chain */
    hdl_dat.chain = NULL;     /* the fix: transfer ownership */
    bit_chain_free (&dat1);   /* free the buffer through dat1 */
  }
  /* hdl_dat.chain is now NULL — freeing it must be a no-op */
  bit_chain_free (&hdl_dat);
  ok ("common_entity_handle_data double-free regression");
}

/* Regression test for GHSA-6q9w-j39m-jg37 (incomplete fix for GH #357 from
   2022, commit 065a3119). remove_NOD_item's first bounds check
     if (i < numitems && itemhandles[i] != NULL)
   dereferenced _obj->itemhandles[i] without first checking
   _obj->itemhandles != NULL, unlike the second check a few lines below
   which already had the guard. A DICTIONARY with numitems > 0 but a NULL
   itemhandles array (a malformed/edge-case state reachable during encode)
   crashed with a NULL-pointer dereference. */
static void
remove_NOD_item_null_itemhandles_test (void)
{
  Dwg_Object_DICTIONARY obj;

  memset (&obj, 0, sizeof (obj));
  obj.numitems = 1;
  obj.itemhandles = NULL;
  obj.texts = NULL;

  remove_NOD_item (&obj, 0, "ACAD_TEST");
  ok ("remove_NOD_item: survives NULL itemhandles with numitems > 0");
}

/* Regression test for GH #1487 (in_json section counts): a count key
   given apart from the buffer it counts must never exceed the buffer.
   AppInfo.size, AppInfoHistory.size, Security.encr_size,
   RevHistory.num_histories and AcDs.num_segidx are now calculated from
   the buffer/array they count. On master, a large count after the
   buffer made dwg_free_acds walk 4096 entries of the 2-element
   segidx/segments arrays. */
static void
in_json_section_counts_test (void)
{
  const char *src
      = "{\n"
        " \"created_by\": \"libredwg test\",\n"
        " \"AppInfo\": { \"unknown_bits\": \"4142\", \"size\": 4096 },\n"
        " \"AppInfoHistory\": { \"unknown_bits\": \"4142\", \"size\": 4096 "
        "},\n"
        " \"RevHistory\": { \"histories\": [100], \"num_histories\": 4096 },\n"
        " \"AcDs\": { \"segidx\": [ { \"offset\": 0, \"size\": 0 },\n"
        "                { \"offset\": 0, \"size\": 0 } ],\n"
        "   \"segments\": [ { \"signature\": 0 },\n"
        "                  { \"signature\": 0 } ],\n"
        "   \"num_segidx\": 4096 } }";
  char *json = (char *)malloc (strlen (src) + 1);
  Dwg_Data dwg;
  Bit_Chain dat;
  int error;

  if (!json)
    {
      fail ("in_json_section_counts: malloc failed");
      return;
    }
  strcpy (json, src);
  memset (&dwg, 0, sizeof (dwg));
  memset (&dat, 0, sizeof (dat));
  dat.chain = (unsigned char *)json;
  dat.size = strlen (json);
  error = dwg_read_json (&dat, &dwg);
  if (error >= DWG_ERR_CRITICAL)
    {
      fail ("in_json_section_counts: dwg_read_json failed 0x%x", error);
      free (json);
      return;
    }
  if (dwg.appinfo.size != 2)
    fail ("in_json_section_counts: AppInfo.size %u != 2",
          (unsigned)dwg.appinfo.size);
  if (dwg.appinfohistory.size != 2)
    fail ("in_json_section_counts: AppInfoHistory.size %u != 2",
          (unsigned)dwg.appinfohistory.size);
  if (dwg.revhistory.num_histories != 1)
    fail ("in_json_section_counts: RevHistory.num_histories %u != 1",
          (unsigned)dwg.revhistory.num_histories);
  if (dwg.acds.num_segidx != 2)
    fail ("in_json_section_counts: AcDs.num_segidx %u != 2",
          (unsigned)dwg.acds.num_segidx);
  // on master: dwg_free_acds walks 4096 segidx/segments entries past the
  // 2-element arrays
  dwg_free (&dwg);
  free (json);
  ok ("in_json_section_counts: section counts stay within their buffers");
}

/* Regression test for GH #1487 (in_json object counts): object
   count/size keys given after the buffer they count are ignored, the
   buffer sets the count; arrays lower a count given before them.
   LTYPE.numdashes follows dashes, DICTIONARY.numitems the items map,
   BLOCK_HEADER.preview_size the preview buffer. On master the large
   count survived and encode/free walked past the short buffers. */
static void
in_json_object_counts_test (void)
{
  const char *src
      = "{\n"
        " \"created_by\": \"libredwg test\",\n"
        " \"OBJECTS\": [ { \"object\": \"LTYPE\", \"type\": 57,\n"
        "   \"handle\": [0, 1, 1], \"name\": \"TestLTYPE\",\n"
        "   \"dashes\": [ { \"length\": 1.0 } ], \"numdashes\": 200 },\n"
        " { \"object\": \"DICTIONARY\", \"type\": 42,\n"
        "   \"handle\": [0, 1, 2], \"name\": \"testdict\",\n"
        "   \"items\": { \"A\": [2, 1, 1, 1], \"B\": [2, 1, 2, 2] },\n"
        "   \"numitems\": 4096 },\n"
        " { \"object\": \"BLOCK_HEADER\", \"type\": 49,\n"
        "   \"handle\": [0, 1, 3], \"name\": \"*Model_Space\",\n"
        "   \"preview\": \"4142\", \"preview_size\": 3000 } ] }";
  char *json = (char *)malloc (strlen (src) + 1);
  Dwg_Data dwg;
  Bit_Chain dat;
  int error;

  if (!json)
    {
      fail ("in_json_object_counts: malloc failed");
      return;
    }
  strcpy (json, src);
  memset (&dwg, 0, sizeof (dwg));
  memset (&dat, 0, sizeof (dat));
  dat.chain = (unsigned char *)json;
  dat.size = strlen (json);
  error = dwg_read_json (&dat, &dwg);
  if (error >= DWG_ERR_CRITICAL)
    {
      fail ("in_json_object_counts: dwg_read_json failed 0x%x", error);
      free (json);
      return;
    }
  if (dwg.num_objects != 3)
    fail ("in_json_object_counts: num_objects %u != 3",
          (unsigned)dwg.num_objects);
  else
    {
      Dwg_Object_LTYPE *lt = dwg.object[0].tio.object->tio.LTYPE;
      Dwg_Object_DICTIONARY *dict = dwg.object[1].tio.object->tio.DICTIONARY;
      Dwg_Object_BLOCK_HEADER *bh = dwg.object[2].tio.object->tio.BLOCK_HEADER;
      if (lt->numdashes != 1)
        fail ("in_json_object_counts: LTYPE.numdashes %u != 1",
              (unsigned)lt->numdashes);
      if (dict->numitems != 2)
        fail ("in_json_object_counts: DICTIONARY.numitems %u != 2",
              (unsigned)dict->numitems);
      if (bh->preview_size != 2)
        fail ("in_json_object_counts: preview_size %u != 2",
              (unsigned)bh->preview_size);
    }
  dwg_free (&dwg);
  free (json);
  ok ("in_json_object_counts: object counts stay within their buffers");
}

/* Regression test for GH #1487 (in_json subclass fixed elems): subclass
   arrays the specs walk a fixed number of times are allocated at least
   that many elements, zero-filled: DIMASSOC.ref is walked 6 times
   (REPEAT_CN), TABLESTYLE.rowstyles 3 (and free_TABLESTYLE_r2010), their
   borders 6. On master a shorter JSON array left the walkers to read
   past the buffer, and dwg_free freed the pointers found there. */
static void
in_json_subclass_fixed_elems_test (void)
{
  const char *src
      = "{\n"
        " \"created_by\": \"libredwg test\",\n"
        " \"FILEHEADER\": { \"version\": \"AC1024\" },\n"
        " \"OBJECTS\": [ { \"object\": \"DIMASSOC\", \"type\": 626,\n"
        "   \"handle\": [0, 1, 1],\n"
        "   \"associativity\": 63,\n"
        "   \"ref\": [ {}, {}, {}, {} ] },\n"
        " { \"object\": \"TABLESTYLE\", \"type\": 724,\n"
        "   \"handle\": [0, 1, 2], \"name\": \"TestTable\",\n"
        "   \"data_rowstyle\": { \"text_height\": 1.5 } } ] }";
  char *json = (char *)malloc (strlen (src) + 1);
  Dwg_Data dwg;
  Bit_Chain dat;
  int error;

  if (!json)
    {
      fail ("in_json_subclass_fixed_elems: malloc failed");
      return;
    }
  strcpy (json, src);
  memset (&dwg, 0, sizeof (dwg));
  memset (&dat, 0, sizeof (dat));
  dat.chain = (unsigned char *)json;
  dat.size = strlen (json);
  error = dwg_read_json (&dat, &dwg);
  if (error >= DWG_ERR_CRITICAL)
    {
      fail ("in_json_subclass_fixed_elems: dwg_read_json failed 0x%x", error);
      free (json);
      return;
    }
  if (dwg.num_objects != 2)
    fail ("in_json_subclass_fixed_elems: num_objects %u != 2",
          (unsigned)dwg.num_objects);
  else
    {
      Dwg_Object_DIMASSOC *da = dwg.object[0].tio.object->tio.DIMASSOC;
      Dwg_Object_TABLESTYLE *ts = dwg.object[1].tio.object->tio.TABLESTYLE;
      // on master: ref[4]/ref[5] past the 4-element buffer
      if (!da->ref)
        fail ("in_json_subclass_fixed_elems: ref missing");
      else if (da->ref[4].osnap_type != 0 || da->ref[5].osnap_type != 0
               || da->ref[5].classname != NULL
               || ts->data_rowstyle.text_height != 1.5
               || ts->title_rowstyle.text_height != 0.0)
        fail ("in_json_subclass_fixed_elems: fixed elems not zero-filled");
    }
  // on master: dwg_free walks 6 DIMASSOC refs and 3 rowstyles x 6 borders
  // past the shorter buffers, and frees the pointers found there
  dwg_free (&dwg);
  free (json);
  ok ("in_json_subclass_fixed_elems: subclass arrays get the walked elems");
}

/* Regression test for GH #1487 (in_json MLINE lines): the spec sets
   every vertex's num_lines to MLINE.num_lines and walks that many lines
   on each vertex. in_json ended MLINE.num_lines at the last vertex's
   count, while each vertex's lines were allocated at its own count, so
   an earlier vertex with fewer lines left encode, out_dxf, out_json
   and dwg_free to read past it. Now MLINE.num_lines is the most lines
   of any vertex and the earlier vertices are grown, zero-filled. */
static void
in_json_mline_lines_test (void)
{
  const char *src = "{\n"
                    " \"created_by\": \"libredwg test\",\n"
                    " \"OBJECTS\": [ { \"entity\": \"MLINE\", \"type\": 47,\n"
                    "   \"handle\": [0, 1, 1],\n"
                    "   \"verts\": [ { \"lines\": [ {}, {} ] },\n"
                    "                { \"lines\": [ {}, {}, {} ] } ] } ] }";
  char *json = (char *)malloc (strlen (src) + 1);
  Dwg_Data dwg;
  Bit_Chain dat;
  int error;

  if (!json)
    {
      fail ("in_json_mline_lines: malloc failed");
      return;
    }
  strcpy (json, src);
  memset (&dwg, 0, sizeof (dwg));
  memset (&dat, 0, sizeof (dat));
  dat.chain = (unsigned char *)json;
  dat.size = strlen (json);
  error = dwg_read_json (&dat, &dwg);
  if (error >= DWG_ERR_CRITICAL)
    {
      fail ("in_json_mline_lines: dwg_read_json failed 0x%x", error);
      free (json);
      return;
    }
  if (dwg.num_objects != 1)
    fail ("in_json_mline_lines: num_objects %u != 1",
          (unsigned)dwg.num_objects);
  else
    {
      Dwg_Entity_MLINE *o = dwg.object[0].tio.entity->tio.MLINE;
      // on master: MLINE.num_lines is the last vertex's count, and
      // verts[0].lines has 2 elements only
      if (o->num_lines != 3)
        fail ("in_json_mline_lines: MLINE.num_lines %u != 3",
              (unsigned)o->num_lines);
      if (o->verts[0].num_lines != 3)
        fail ("in_json_mline_lines: verts[0].num_lines %u != 3",
              (unsigned)o->verts[0].num_lines);
      if (o->verts[0].lines && o->verts[0].lines[2].num_segparms != 0)
        fail ("in_json_mline_lines: verts[0].lines[2] not zero-filled");
    }
  // on master: dwg_free walks 3 lines of the first vertex past its
  // 2-element lines array
  dwg_free (&dwg);
  free (json);
  ok ("in_json_mline_lines: every MLINE vertex gets num_lines lines");
}

/* Regression test for GH #1487 (in_json dotted keys into unset arrays):
   a dotted key into a subclass array that is not set (e.g.
   "ref.classname" from example_2004.dwg, "data_rowstyle.text_height", or
   "dashes.length" with the array removed) now does what an array of one
   element does: the array gets the elements the specs walk,
   zero-filled, and its count is set to 1 (within the other array when
   shared, as in the LTYPE dashes.numdashes case). On master the specs
   and dwg_free walked past the single element. */
static void
in_json_dotted_array_test (void)
{
  const char *src = "{\n"
                    " \"created_by\": \"libredwg test\",\n"
                    " \"FILEHEADER\": { \"version\": \"AC1024\" },\n"
                    " \"OBJECTS\": ["
                    " { \"object\": \"DIMASSOC\", \"type\": 626,\n"
                    "   \"handle\": [0, 1, 1],\n"
                    "   \"associativity\": 63,\n"
                    "   \"ref.classname\": \"AcDbOsnapPointRef\" },\n"
                    " { \"object\": \"TABLESTYLE\", \"type\": 724,\n"
                    "   \"handle\": [0, 1, 2], \"name\": \"TestTable\",\n"
                    "   \"data_rowstyle.text_height\": 2.5 },\n"
                    " { \"entity\": \"MLINE\", \"type\": 47,\n"
                    "   \"handle\": [0, 1, 3], \"layer\": \"0\",\n"
                    "   \"verts.lines\": [ {}, {} ] },\n"
                    " { \"object\": \"LTYPE\", \"type\": 57,\n"
                    "   \"handle\": [0, 1, 4], \"name\": \"TestLTYPE\",\n"
                    "   \"numdashes\": 5, \"dashes.length\": 5.0 } ] }";
  char *json = (char *)malloc (strlen (src) + 1);
  Dwg_Data dwg;
  Bit_Chain dat;
  int error;

  if (!json)
    {
      fail ("in_json_dotted_array: malloc failed");
      return;
    }
  strcpy (json, src);
  memset (&dwg, 0, sizeof (dwg));
  memset (&dat, 0, sizeof (dat));
  dat.chain = (unsigned char *)json;
  dat.size = strlen (json);
  error = dwg_read_json (&dat, &dwg);
  if (error >= DWG_ERR_CRITICAL)
    {
      fail ("in_json_dotted_array: dwg_read_json failed 0x%x", error);
      free (json);
      return;
    }
  if (dwg.num_objects != 4)
    fail ("in_json_dotted_array: num_objects %u != 4",
          (unsigned)dwg.num_objects);
  else
    {
      BITCODE_BL i;
      for (i = 0; i < dwg.num_objects; i++)
        {
          switch (dwg.object[i].fixedtype)
            {
            case DWG_TYPE_DIMASSOC:
              {
                Dwg_Object_DIMASSOC *o
                    = dwg.object[i].tio.object->tio.DIMASSOC;
                // the specs walk ref 6 times; 1 element would OOB
                if (!o->ref)
                  fail ("in_json_dotted_array: ref not allocated");
                else
                  {
                    if (!o->ref[0].classname
                        || strcmp (o->ref[0].classname, "AcDbOsnapPointRef")
                               != 0)
                      fail ("in_json_dotted_array: ref[0].classname not set");
                    if (o->ref[5].classname || o->ref[5].osnap_type
                        || o->ref[5].xrefs)
                      fail ("in_json_dotted_array: ref[5] not zero-filled");
                  }
                break;
              }
            case DWG_TYPE_TABLESTYLE:
              {
                Dwg_Object_TABLESTYLE *o
                    = dwg.object[i].tio.object->tio.TABLESTYLE;
                // the embedded rowstyles are not allocated
                if (o->data_rowstyle.text_height != 2.5)
                  fail ("in_json_dotted_array: data_rowstyle.text_height "
                        "not set");
                if (o->header_rowstyle.text_height != 0.0)
                  fail ("in_json_dotted_array: header_rowstyle not "
                        "zero-filled");
                break;
              }
            case DWG_TYPE_MLINE:
              {
                Dwg_Entity_MLINE *o = dwg.object[i].tio.entity->tio.MLINE;
                if (o->num_verts != 1)
                  fail ("in_json_dotted_array: num_verts %u != 1",
                        (unsigned)o->num_verts);
                else if (o->verts[0].num_lines != 2 || o->num_lines != 2)
                  fail ("in_json_dotted_array: vertex lines %u != 2",
                        (unsigned)o->verts[0].num_lines);
                break;
              }
            case DWG_TYPE_LTYPE:
              {
                Dwg_Object_LTYPE *o = dwg.object[i].tio.object->tio.LTYPE;
                // the count given before the buffer is lowered by the
                // 1-element array it is now shared with
                if (o->numdashes != 1)
                  fail ("in_json_dotted_array: numdashes %u != 1",
                        (unsigned)o->numdashes);
                if (o->dashes && o->dashes[0].length != 5.0)
                  fail ("in_json_dotted_array: dashes[0].length not set");
                break;
              }
            default:
              break;
            }
        }
    }
  // on master: dwg_free walks the fixed elems past the 1-element
  // arrays
  dwg_free (&dwg);
  free (json);
  ok ("in_json_dotted_array: dotted keys into unset arrays get the "
      "spec-walked elements");
}

static int read_test_data (const char *name, Bit_Chain *file);

/* A test-data file as a NUL-terminated text. */
static char *
dxf_test_data_text (const char *name)
{
  Bit_Chain file;
  char *text;
  if (read_test_data (name, &file))
    return NULL;
  text = (char *)realloc (file.chain, file.size + 1);
  if (!text)
    {
      free (file.chain);
      return NULL;
    }
  text[file.size] = '\0';
  return text;
}

/* The DXF of a test-data drawing at the given version, as text. */
static char *
dxf_of_test_data (const char *name, const Dwg_Version_Type version)
{
  static const char out[] = "encode_test_out.dxf";
  const unsigned level = loglevel;
  Dwg_Data dwg = { 0 };
  Bit_Chain file;
  char *text = NULL;
  int error;
  if (read_test_data (name, &file))
    return NULL;
  loglevel = 0; // a whole drawing
  error = dwg_decode (&file, &dwg);
  free (file.chain);
  if (error < DWG_ERR_CRITICAL)
    {
      FILE *fp;
      dwg.header.from_version = dwg.header.version;
      dwg.header.version = version;
      remove (out);
      error = dxf_write_file (out, &dwg, 0);
      fp = error < DWG_ERR_CRITICAL ? fopen (out, "rb") : NULL;
      if (fp)
        {
          long len;
          if (!fseek (fp, 0, SEEK_END) && (len = ftell (fp)) > 0
              && !fseek (fp, 0, SEEK_SET)
              && (text = (char *)malloc ((size_t)len + 1)))
            {
              if (fread (text, 1, (size_t)len, fp) == (size_t)len)
                text[len] = '\0';
              else
                {
                  free (text);
                  text = NULL;
                }
            }
          fclose (fp);
        }
      remove (out);
    }
  dwg_free (&dwg);
  loglevel = level;
  return text;
}

/* The next group of a DXF text: its code and its value without the line
   end. Returns 0 at the end. */
static int
dxf_group (const char **pp, int *code, const char **value, size_t *len)
{
  const char *p = *pp, *e;
  char *end;
  *code = (int)strtol (p, &end, 10);
  if (end == p || !(p = strchr (end, '\n')))
    return 0;
  p++;
  e = strchr (p, '\n');
  if (!e)
    e = p + strlen (p);
  *value = p;
  *len = (size_t)(e - p);
  if (*len && p[*len - 1] == '\r')
    (*len)--;
  *pp = *e ? e + 1 : e;
  return 1;
}

/* Whether a group value is s. */
static int
dxf_is (const char *value, const size_t len, const char *s)
{
  return strlen (s) == len && !memcmp (value, s, len);
}

/* Appends "name|" to a list of names, once. */
static void
dxf_list_add (char *list, const size_t listsize, const char *value,
              const size_t len)
{
  char item[80];
  if (len + 3 > sizeof (item))
    return;
  item[0] = '|';
  memcpy (&item[1], value, len);
  item[len + 1] = '|';
  item[len + 2] = '\0';
  if (!strstr (list, item) && strlen (list) + len + 2 < listsize)
    {
      if (!*list)
        strcpy (list, "|");
      strcat (list, &item[1]);
    }
}

/* Whether two |name| lists have the same names, in any order. */
static int
dxf_list_same (const char *a, const char *b)
{
  const char *p;
  for (p = a; *p && p[1]; p = strchr (p + 1, '|'))
    {
      char item[80];
      const char *e = strchr (p + 1, '|');
      if (!e || (size_t)(e - p) + 2 > sizeof (item))
        return 0;
      memcpy (item, p, (size_t)(e - p) + 1);
      item[e - p + 1] = '\0';
      if (!strstr (b, item))
        return 0;
    }
  return strlen (a) == strlen (b);
}

/* What the DXF checks compare. */
typedef struct
{
  long maintver;
  char sections[100];  // |HEADER|TABLES|...
  char types[600];     // the entity types in BLOCKS and ENTITIES
  char layers[300];    // the LAYER names
  char viewports[200]; // |handle=id| per VIEWPORT
  unsigned rotated;    // DIMENSION_LINEAR (AcDbRotatedDimension)
  unsigned dim52;      // 52 groups in DIMENSIONs
  unsigned mtext;      // MTEXTs
  unsigned mtext101;   // MTEXTs with an embedded object
  unsigned pspace;     // entities with 67 1
  unsigned vports;     // VIEWPORTs
  unsigned dangling;   // DICTIONARY entries to objects not in the DXF
} dxf_features;

/* The DICTIONARY entries (350, 360) to a handle no object of the DXF has. */
static unsigned
dxf_dangling_entries (const char *text)
{
  const char *p = text, *value;
  size_t len, nhandles = 0, cap = 0;
  int code;
  unsigned long *handles = NULL;
  unsigned dangling = 0;
  char cur[40] = "";
  while (dxf_group (&p, &code, &value, &len))
    if (code == 5 || code == 105)
      {
        if (nhandles == cap)
          {
            unsigned long *h;
            cap = cap ? 2 * cap : 1024;
            h = (unsigned long *)realloc (handles, cap * sizeof (*h));
            if (!h)
              {
                free (handles);
                return 0;
              }
            handles = h;
          }
        handles[nhandles++] = strtoul (value, NULL, 16);
      }
  p = text;
  while (dxf_group (&p, &code, &value, &len))
    if (code == 0)
      snprintf (cur, sizeof (cur), "%.*s", (int)len, value);
    else if ((code == 350 || code == 360) && !strncmp (cur, "DICTIONARY", 10))
      {
        const unsigned long h = strtoul (value, NULL, 16);
        size_t i;
        for (i = 0; i < nhandles; i++)
          if (handles[i] == h)
            break;
        if (h && i == nhandles)
          dangling++;
      }
  free (handles);
  return dangling;
}

static void
dxf_features_of (const char *text, dxf_features *f)
{
  const char *p = text, *value;
  size_t len;
  int code, prev_code = -1;
  char cur[40] = "", sec[20] = "", vp_handle[20] = "";
  int in_mtext101 = 0;
  memset (f, 0, sizeof (dxf_features));
  f->maintver = -1;
  f->dangling = dxf_dangling_entries (text);
  while (dxf_group (&p, &code, &value, &len))
    {
      if (code == 0)
        {
          if (dxf_is (value, len, "MTEXT"))
            f->mtext++;
          else if (dxf_is (value, len, "VIEWPORT"))
            f->vports++;
          in_mtext101 = 0;
          snprintf (cur, sizeof (cur), "%.*s", (int)len, value);
          if (!strcmp (cur, "SECTION"))
            sec[0] = '\0';
          else if ((!strcmp (sec, "BLOCKS") || !strcmp (sec, "ENTITIES"))
                   && strcmp (cur, "BLOCK") && strcmp (cur, "ENDBLK")
                   && strcmp (cur, "ENDSEC") && strcmp (cur, "EOF"))
            dxf_list_add (f->types, sizeof (f->types), value, len);
        }
      else if (code == 2 && !strcmp (cur, "SECTION") && !sec[0])
        {
          snprintf (sec, sizeof (sec), "%.*s", (int)len, value);
          dxf_list_add (f->sections, sizeof (f->sections), value, len);
        }
      else if (code == 2 && !strcmp (cur, "LAYER") && !strcmp (sec, "TABLES"))
        dxf_list_add (f->layers, sizeof (f->layers), value, len);
      else if (code == 9)
        prev_code = dxf_is (value, len, "$ACADMAINTVER") ? 9 : -1;
      else if (prev_code == 9 && (code == 70 || code == 90))
        {
          f->maintver = strtol (value, NULL, 10);
          prev_code = -1;
        }
      else if (code == 5 && !strcmp (cur, "VIEWPORT"))
        snprintf (vp_handle, sizeof (vp_handle), "%.*s", (int)len, value);
      else if (code == 69 && !strcmp (cur, "VIEWPORT"))
        {
          char item[40];
          snprintf (item, sizeof (item), "%s=%ld", vp_handle,
                    strtol (value, NULL, 10));
          dxf_list_add (f->viewports, sizeof (f->viewports), item,
                        strlen (item));
        }
      else if (code == 100 && !strcmp (cur, "DIMENSION")
               && dxf_is (value, len, "AcDbRotatedDimension"))
        f->rotated++;
      else if (code == 52 && !strcmp (cur, "DIMENSION"))
        f->dim52++;
      else if (code == 67 && strtol (value, NULL, 10) == 1)
        f->pspace++;
      else if (code == 101 && !strcmp (cur, "MTEXT") && !in_mtext101)
        {
          in_mtext101 = 1;
          f->mtext101++;
        }
    }
}

/* example_2018.dwg as DXF of each version, against AutoCAD 2018's DXF of
   the same drawing (example_2000.dxf .. example_2018.dxf). */
static void
dxf_as_acad_test (void)
{
  static const struct
  {
    const char *acad;
    Dwg_Version_Type version;
  } t[] = { { "example_2000.dxf", R_2000 }, { "example_2004.dxf", R_2004 },
            { "example_2007.dxf", R_2007 }, { "example_2010.dxf", R_2010 },
            { "example_2013.dxf", R_2013 }, { "example_2018.dxf", R_2018 } };
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (t); i++)
    {
      const char *v = dwg_version_type (t[i].version);
      char *acad_text = dxf_test_data_text (t[i].acad);
      char *ours_text = dxf_of_test_data ("example_2018.dwg", t[i].version);
      dxf_features a, o;
      if (!acad_text || !ours_text)
        {
          fail ("DXF %s: %s", v, acad_text ? "not written" : "no fixture");
          free (acad_text);
          free (ours_text);
          continue;
        }
      dxf_features_of (acad_text, &a);
      dxf_features_of (ours_text, &o);
      free (acad_text);
      free (ours_text);
      /* the product maintenance version, AutoCAD 2018's and AutoCAD LT
         2027's alike for r2004-r2013 */
      if (t[i].version >= R_2004 && t[i].version < R_2018)
        {
          if (o.maintver == a.maintver)
            ok ("DXF %s: $ACADMAINTVER %ld", v, o.maintver);
          else
            fail ("DXF %s: $ACADMAINTVER %ld, AutoCAD %ld", v, o.maintver,
                  a.maintver);
        }
      /* the current layout's viewports numbered, the others off */
      if (strEQ (o.viewports, a.viewports))
        ok ("DXF %s: VIEWPORT ids %s", v, o.viewports);
      else
        fail ("DXF %s: VIEWPORT ids %s, AutoCAD %s", v, o.viewports,
              a.viewports);
      if (o.rotated == a.rotated && o.dim52 == a.dim52)
        ok ("DXF %s: %u rotated dimensions, %u with 52", v, o.rotated,
            o.dim52);
      else
        fail ("DXF %s: %u rotated dimensions, %u with 52, AutoCAD %u, %u", v,
              o.rotated, o.dim52, a.rotated, a.dim52);
      /* the embedded object only for MTEXT with columns */
      if (o.mtext == a.mtext && o.mtext101 == a.mtext101)
        ok ("DXF %s: %u MTEXT, %u with an embedded object", v, o.mtext,
            o.mtext101);
      else
        fail ("DXF %s: %u MTEXT, %u with an embedded object, AutoCAD %u, %u",
              v, o.mtext, o.mtext101, a.mtext, a.mtext101);
      /* 67 1 for the entities of every paper space layout */
      if (o.pspace == a.pspace)
        ok ("DXF %s: %u entities with 67 1", v, o.pspace);
      else
        fail ("DXF %s: %u entities with 67 1, AutoCAD %u", v, o.pspace,
              a.pspace);
      if (dxf_list_same (o.layers, a.layers))
        ok ("DXF %s: layers %s", v, o.layers);
      else
        fail ("DXF %s: layers %s, AutoCAD %s", v, o.layers, a.layers);
      if (!o.dangling)
        ok ("DXF %s: no DICTIONARY entry to an absent object", v);
      else
        fail ("DXF %s: %u DICTIONARY entries to absent objects", v,
              o.dangling);
    }
}

/* example_2018.dwg as R12 DXF, against AutoCAD 2018's (example_r12.dxf):
   the sections, the entity types and the R12 names, and it reads back. */
static void
dxf_r12_test (void)
{
  static const char out[] = "encode_test_r12.dxf";
  char *acad_text = dxf_test_data_text ("example_r12.dxf");
  char *ours_text = dxf_of_test_data ("example_2018.dwg", R_12);
  dxf_features a, o;
  if (!acad_text || !ours_text)
    {
      fail ("DXF r12: %s", acad_text ? "not written" : "no fixture");
      free (acad_text);
      free (ours_text);
      return;
    }
  dxf_features_of (acad_text, &a);
  dxf_features_of (ours_text, &o);
  free (acad_text);
  if (strEQ (o.sections, a.sections))
    ok ("DXF r12: sections %s", o.sections);
  else
    fail ("DXF r12: sections %s, AutoCAD %s", o.sections, a.sections);
  if (dxf_list_same (o.types, a.types))
    ok ("DXF r12: entity types %s", o.types);
  else
    fail ("DXF r12: entity types %s, AutoCAD %s", o.types, a.types);
  if (strEQ (o.layers, a.layers))
    ok ("DXF r12: layers %s", o.layers);
  else
    fail ("DXF r12: layers %s, AutoCAD %s", o.layers, a.layers);
  /* the viewports of the current layout only */
  if (o.vports == a.vports)
    ok ("DXF r12: %u VIEWPORTs", o.vports);
  else
    fail ("DXF r12: %u VIEWPORTs, AutoCAD %u", o.vports, a.vports);
  {
    Dwg_Data back = { 0 };
    FILE *fp;
    int error = DWG_ERR_IOERROR;
    remove (out);
    fp = fopen (out, "wb");
    if (fp)
      {
        fwrite (ours_text, 1, strlen (ours_text), fp);
        fclose (fp);
        loglevel = 0; // a whole drawing
        error = dxf_read_file (out, &back);
        loglevel = is_make_silent () ? 0 : 3;
      }
    remove (out);
    if (error >= DWG_ERR_CRITICAL)
      fail ("DXF r12: read back: 0x%x", error);
    else
      ok ("DXF r12: read back, %u objects", (unsigned)back.num_objects);
    dwg_free (&back);
  }
  free (ours_text);
}

/* in_dxf: an empty 1000 string of an r2007+ XRECORD, as AutoCAD's r2007+
   DXF has them, is kept as an empty string. */
static void
in_dxf_empty_xdata_test (void)
{
  static const char dxf[]
      = "  0\nSECTION\n  2\nHEADER\n  9\n$ACADVER\n  1\nAC1032\n"
        "  9\n$HANDSEED\n  5\n200\n"
        "  9\n$EXTMIN\n 10\n0.0\n 20\n0.0\n 30\n0.0\n"
        "  9\n$EXTMAX\n 10\n100.0\n 20\n100.0\n 30\n0.0\n"
        "  0\nENDSEC\n  0\nSECTION\n  2\nOBJECTS\n"
        "  0\nXRECORD\n  5\n100\n330\n0\n100\nAcDbXrecord\n280\n1\n"
        "1000\n\n1000\nx\n  0\nENDSEC\n  0\nEOF\n";
  Dwg_Data dwg = { 0 };
  Bit_Chain dat = { 0 };
  const Dwg_Object_XRECORD *xrec = NULL;
  const Dwg_Resbuf *rb;
  unsigned n = 0, empty = 0;
  BITCODE_BL i;
  int error;
  dat.chain = (unsigned char *)malloc (sizeof (dxf));
  if (!dat.chain)
    {
      fail ("in_dxf empty xdata: malloc failed");
      return;
    }
  memcpy (dat.chain, dxf, sizeof (dxf));
  dat.size = sizeof (dxf) - 1;
  loglevel = 0;
  error = dwg_read_dxf (&dat, &dwg);
  loglevel = is_make_silent () ? 0 : 3;
  free (dat.chain);
  for (i = 0; error < DWG_ERR_CRITICAL && i < dwg.num_objects; i++)
    if (dwg.object[i].fixedtype == DWG_TYPE_XRECORD)
      xrec = dwg.object[i].tio.object->tio.XRECORD;
  for (rb = xrec ? xrec->xdata : NULL; rb && n < 10; rb = rb->nextrb, n++)
    if (rb->type == 1000 && rb->value.str.u.data && !rb->value.str.size)
      empty++;
  if (n == 2 && empty == 1)
    ok ("in_dxf: an empty 1000 string in an XRECORD");
  else
    fail ("in_dxf: XRECORD with %u xdata, %u empty strings (error 0x%x)", n,
          empty, error);
  dwg_free (&dwg);
}

// R2010+ TABLE -> pre-R2010 cells: text from the UTF-16LE value, merged cells
// as in AutoCAD's own pre-R2010 tables (top left holds the extent, the
// covered cells are marked merged), column widths and row heights.
static void
downconvert_TABLE_test (void)
{
  Dwg_Data dwg;
  Dwg_Object obj;
  Dwg_Object_Entity ent;
  Dwg_Entity_TABLE table;
  Dwg_TableDataColumn cols[3];
  Dwg_TableRow rows[2];
  Dwg_TableCell row0[3], row1[3];
  Dwg_TableCellContent c00, c11;
  Dwg_FormattedTableMerged merged;
  // "ab" with the terminating NUL
  unsigned char ab[6] = { 'a', 0, 'b', 0, 0, 0 };
  BITCODE_RS *tu;

  memset (&dwg, 0, sizeof dwg);
  memset (&obj, 0, sizeof obj);
  memset (&ent, 0, sizeof ent);
  memset (&table, 0, sizeof table);
  memset (cols, 0, sizeof cols);
  memset (rows, 0, sizeof rows);
  memset (row0, 0, sizeof row0);
  memset (row1, 0, sizeof row1);
  memset (&c00, 0, sizeof c00);
  memset (&c11, 0, sizeof c11);
  dwg.header.from_version = R_2013;
  dwg.header.version = R_2013;
  obj.parent = &dwg;
  obj.fixedtype = DWG_TYPE_TABLE;
  obj.tio.entity = &ent;
  ent.tio.TABLE = &table;
  table.rotation = 0.0;
  table.tdata.num_cols = 3;
  table.tdata.cols = cols;
  table.tdata.num_rows = 2;
  table.tdata.rows = rows;
  cols[0].width = 10.0;
  cols[1].width = 20.0;
  cols[2].width = 30.0;
  rows[0].height = 4.0;
  rows[1].height = 5.0;
  rows[0].num_cells = rows[1].num_cells = 3;
  rows[0].cells = row0;
  rows[1].cells = row1;
  c00.type = 1;
  c00.value.data_type = 4;
  c00.value.data_size = sizeof ab;
  c00.value.data_date = (BITCODE_TF)ab;
  row0[0].num_cell_contents = 1;
  row0[0].cell_contents = &c00;
  c11.type = 1;
  c11.value.data_type = 1;
  c11.value.data_long = 42;
  row1[1].num_cell_contents = 1;
  row1[1].cell_contents = &c11;
  // merge the first row
  merged.top_row = 0;
  merged.left_col = 0;
  merged.bottom_row = 0;
  merged.right_col = 2;
  table.fdata.num_merged_cells = 1;
  table.fdata.merged_cells = &merged;

  downconvert_TABLE (&obj);

  if (table.num_cols != 3 || table.num_rows != 2 || table.num_cells != 6
      || !table.cells || !table.col_widths || !table.row_heights)
    {
      fail ("downconvert_TABLE: no cells");
      return;
    }
  if (table.col_widths[1] != 20.0 || table.row_heights[1] != 5.0)
    fail ("downconvert_TABLE: widths/heights");
  tu = (BITCODE_RS *)table.cells[0].text_value;
  if (!tu || tu[0] != 'a' || tu[1] != 'b' || tu[2] != 0)
    fail ("downconvert_TABLE: cell 0 text is not the UTF-16 \"ab\"");
  tu = (BITCODE_RS *)table.cells[4].text_value;
  if (!tu || tu[0] != '4' || tu[1] != '2' || tu[2] != 0)
    fail ("downconvert_TABLE: number cell text");
  if (table.cells[0].merged_width_flag != 3
      || table.cells[0].merged_height_flag != 1
      || table.cells[0].is_merged_value)
    fail ("downconvert_TABLE: merge master %u x %u merged %d",
          (unsigned)table.cells[0].merged_width_flag,
          (unsigned)table.cells[0].merged_height_flag,
          (int)table.cells[0].is_merged_value);
  if (!table.cells[1].is_merged_value || !table.cells[2].is_merged_value
      || table.cells[3].is_merged_value)
    fail ("downconvert_TABLE: covered cells");
  if (table.cells[5].merged_width_flag != 1
      || table.cells[5].merged_height_flag != 1)
    fail ("downconvert_TABLE: plain cell extent");
  if (table.horiz_direction.x != 1.0 || table.horiz_direction.y != 0.0)
    fail ("downconvert_TABLE: horiz_direction");
  if (table.flag_for_table_value != 0x16)
    fail ("downconvert_TABLE: flag_for_table_value 0x%x",
          (unsigned)table.flag_for_table_value);
  // not twice, and free what was made
  downconvert_TABLE (&obj);
  for (unsigned i = 0; i < table.num_cells; i++)
    {
      free (table.cells[i].text_value);
      free (table.cells[i].value.data_date);
      free (table.cells[i].value.value_string);
    }
  free (table.cells);
  free (table.col_widths);
  free (table.row_heights);
  ok ("downconvert_TABLE: text, merged cells and extents");
}

// R2010+ TABLESTYLE cell styles (title 1, header 2, data 3) to the pre-R2010
// rowstyles (0: data, 1: title, 2: header) and back.
static void
downconvert_TABLESTYLE_test (void)
{
  Dwg_Data dwg;
  Dwg_Object obj;
  Dwg_Object_Object oo;
  Dwg_Object_TABLESTYLE ts, back;
  Dwg_TABLESTYLE_cellstyle cs[3];
  Dwg_GridFormat data_borders[2];
  Dwg_GridFormat title_borders[1];
  Dwg_Object_Object oo2;
  Dwg_Object obj2;

  memset (&dwg, 0, sizeof dwg);
  memset (&obj, 0, sizeof obj);
  memset (&oo, 0, sizeof oo);
  memset (&ts, 0, sizeof ts);
  memset (&back, 0, sizeof back);
  memset (cs, 0, sizeof cs);
  memset (data_borders, 0, sizeof data_borders);
  memset (title_borders, 0, sizeof title_borders);
  memset (&oo2, 0, sizeof oo2);
  memset (&obj2, 0, sizeof obj2);
  dwg.header.from_version = R_2018;
  dwg.header.version = R_2018;
  obj.parent = &dwg;
  obj.fixedtype = DWG_TYPE_TABLESTYLE;
  obj.tio.object = &oo;
  oo.tio.TABLESTYLE = &ts;
  ts.sty.cellstyle.margin_override_flags = 1;
  ts.sty.cellstyle.horiz_margin = 1.5;
  ts.sty.cellstyle.vert_margin = 2.0;
  // title: with a background color
  cs[0].id = 1;
  cs[0].cellstyle.content_format.text_height = 6.0;
  cs[0].cellstyle.content_format.cell_alignment = 5;
  cs[0].cellstyle.bg_color.rgb = 0xc2123456;
  cs[0].cellstyle.num_borders = 1;
  cs[0].cellstyle.borders = title_borders;
  title_borders[0].index_mask = 0x20; // inside horizontal
  title_borders[0].linewt = 50;
  // header
  cs[1].id = 2;
  cs[1].cellstyle.content_format.text_height = 4.5;
  cs[1].cellstyle.content_format.cell_alignment = 5;
  cs[1].cellstyle.bg_color.rgb = 0xc8000000; // none
  // data: the top border is invisible, the right border is visible
  cs[2].id = 3;
  cs[2].cellstyle.content_format.text_height = 4.5;
  cs[2].cellstyle.content_format.cell_alignment = 2;
  cs[2].cellstyle.bg_color.rgb = 0xc8000000;
  cs[2].cellstyle.num_borders = 2;
  cs[2].cellstyle.borders = data_borders;
  data_borders[0].index_mask = 1;
  data_borders[0].linewt = 25;
  data_borders[0].visible = 1; // BL 93: invisible
  data_borders[1].index_mask = 2;
  data_borders[1].linewt = 35;
  data_borders[1].visible = 0;
  ts.num_cellstyles = 3;
  ts.cellstyles = cs;

  downconvert_TABLESTYLE (&obj);

  if (ts.data_rowstyle.text_height != 4.5
      || ts.title_rowstyle.text_height != 6.0
      || ts.header_rowstyle.text_height != 4.5)
    fail ("downconvert_TABLESTYLE: order data, title, header");
  if (ts.data_rowstyle.text_alignment != 2
      || ts.title_rowstyle.text_alignment != 5)
    fail ("downconvert_TABLESTYLE: alignment");
  if (ts.data_rowstyle.has_bgcolor || ts.data_rowstyle.fill_color.index != 256
      || !ts.title_rowstyle.has_bgcolor
      || ts.title_rowstyle.fill_color.rgb != 0xc2123456)
    fail ("downconvert_TABLESTYLE: fill color");
  // pre-R2010 border order: top, horizontal inside, bottom, left,
  // vertical inside, right
  if (ts.data_rowstyle.top_border.linewt != 25
      || ts.data_rowstyle.top_border.visible != 0
      || ts.data_rowstyle.right_border.linewt != 35
      || ts.data_rowstyle.right_border.visible != 1
      || ts.data_rowstyle.bot_border.linewt != -2
      || ts.data_rowstyle.bot_border.visible != 1)
    fail ("downconvert_TABLESTYLE: data borders");
  if (ts.title_rowstyle.hor_border.linewt != 50)
    fail ("downconvert_TABLESTYLE: title horizontal inside border");
  if (ts.horiz_cell_margin != 1.5 || ts.vert_cell_margin != 2.0)
    fail ("downconvert_TABLESTYLE: margins");

  // and back
  dwg.header.from_version = R_2000;
  dwg.header.version = R_2000;
  obj2.parent = &dwg;
  obj2.fixedtype = DWG_TYPE_TABLESTYLE;
  obj2.tio.object = &oo2;
  oo2.tio.TABLESTYLE = &back;
  back.data_rowstyle = ts.data_rowstyle;
  back.title_rowstyle = ts.title_rowstyle;
  back.header_rowstyle = ts.header_rowstyle;
  back.horiz_cell_margin = 1.5;
  back.vert_cell_margin = 2.0;
  upconvert_TABLESTYLE (&obj2);
  if (back.num_cellstyles != 3 || !back.cellstyles)
    fail ("upconvert_TABLESTYLE: no cellstyles");
  else
    {
      Dwg_TABLESTYLE_cellstyle *t = &back.cellstyles[0];
      Dwg_TABLESTYLE_cellstyle *h = &back.cellstyles[1];
      Dwg_TABLESTYLE_cellstyle *d = &back.cellstyles[2];
      if (t->id != 1 || h->id != 2 || d->id != 3 || t->type != 1
          || d->type != 2)
        fail ("upconvert_TABLESTYLE: ids %u %u %u", (unsigned)t->id,
              (unsigned)h->id, (unsigned)d->id);
      if (t->cellstyle.merge_flags != 0x8000 || h->cellstyle.merge_flags)
        fail ("upconvert_TABLESTYLE: merge flags");
      if (t->cellstyle.content_format.text_height != 6.0
          || d->cellstyle.content_format.cell_alignment != 2)
        fail ("upconvert_TABLESTYLE: content formats");
      if (t->cellstyle.bg_color.rgb != 0xc2123456
          || d->cellstyle.bg_color.rgb != 0xc8000000)
        fail ("upconvert_TABLESTYLE: background colors");
      // the borders are in the cell style edge flag order 1, 2, 4, 8, ..
      if (d->cellstyle.num_borders != 6
          || d->cellstyle.borders[0].index_mask != 1
          || d->cellstyle.borders[0].linewt != 25
          || d->cellstyle.borders[0].visible != 1 // invisible
          || d->cellstyle.borders[1].index_mask != 2
          || d->cellstyle.borders[1].visible != 0
          || t->cellstyle.borders[5].index_mask != 0x20
          || t->cellstyle.borders[5].linewt != 50)
        fail ("upconvert_TABLESTYLE: borders");
      if (back.sty.cellstyle.vert_margin != 2.0
          || back.sty.cellstyle.horiz_margin != 1.5)
        fail ("upconvert_TABLESTYLE: margins");
    }

  for (unsigned i = 0; i < back.num_cellstyles; i++)
    {
      Dwg_CellStyle *c = &back.cellstyles[i].cellstyle;
      for (unsigned j = 0; j < c->num_borders; j++)
        free (c->borders[j].ltype);
      free (c->borders);
    }
  free (back.sty.cellstyle.content_format.text_style);
  free (back.cellstyles);
  free (ts.data_rowstyle.format_string);
  free (ts.title_rowstyle.format_string);
  free (ts.header_rowstyle.format_string);
  ok ("downconvert_TABLESTYLE: rowstyles and back");
}

// Without the standard data style, the first custom cell style is the data
// rowstyle.
static void
downconvert_TABLESTYLE_custom_test (void)
{
  Dwg_Data dwg;
  Dwg_Object obj;
  Dwg_Object_Object oo;
  Dwg_Object_TABLESTYLE ts;
  Dwg_TABLESTYLE_cellstyle cs[2];

  memset (&dwg, 0, sizeof dwg);
  memset (&obj, 0, sizeof obj);
  memset (&oo, 0, sizeof oo);
  memset (&ts, 0, sizeof ts);
  memset (cs, 0, sizeof cs);
  dwg.header.from_version = R_2018;
  dwg.header.version = R_2018;
  obj.parent = &dwg;
  obj.fixedtype = DWG_TYPE_TABLESTYLE;
  obj.tio.object = &oo;
  oo.tio.TABLESTYLE = &ts;
  cs[0].id = 1;
  cs[0].cellstyle.content_format.text_height = 6.0;
  cs[0].cellstyle.bg_color.rgb = 0xc8000000;
  cs[1].id = 10; // custom
  cs[1].cellstyle.content_format.text_height = 3.5;
  cs[1].cellstyle.bg_color.rgb = 0xc8000000;
  ts.num_cellstyles = 2;
  ts.cellstyles = cs;
  downconvert_TABLESTYLE (&obj);
  if (ts.data_rowstyle.text_height != 3.5
      || ts.title_rowstyle.text_height != 6.0
      || ts.header_rowstyle.text_height != 3.5)
    fail ("downconvert_TABLESTYLE_custom: data %f title %f",
          ts.data_rowstyle.text_height, ts.title_rowstyle.text_height);
  free (ts.data_rowstyle.format_string);
  free (ts.title_rowstyle.format_string);
  free (ts.header_rowstyle.format_string);
  ok ("downconvert_TABLESTYLE_custom: custom cell style as data rowstyle");
}

/* The SAB data of ACIS entities is in the AcDs section since r2013. It is
   written from the entities, and read back by the handle, for small data
   directly in the _data_ segment, for large data (> 0x40000) in a blob01
   segment. */
static void
acds_sab_test (void)
{
  const BITCODE_BL sizes[3] = { 5000, 0x40001, 0x90000 };
  const unsigned saved_loglevel = loglevel;
  char msg[160] = "";
  Dwg_Data dwg, dwg2;
  Bit_Chain dat;
  Dwg_Object *mspace;
  unsigned num_regions = 0, found = 0;
  int error;

  memset (&dwg, 0, sizeof dwg);
  memset (&dwg2, 0, sizeof dwg2);
  memset (&dat, 0, sizeof dat);
  dwg.header.version = dwg.header.from_version = R_2018;
  error = dwg_add_Document (&dwg, 0);
  mspace = error ? NULL : dwg_model_space_object (&dwg);
  if (!mspace)
    snprintf (msg, sizeof msg, "no document");
  for (unsigned i = 0; !*msg && i < 3; i++)
    {
      unsigned char *sab = (unsigned char *)malloc (sizes[i]);
      Dwg_Entity_REGION *reg
          = dwg_add_REGION (mspace->tio.object->tio.BLOCK_HEADER, "dummy");
      Dwg_Object *obj = &dwg.object[dwg.num_objects - 1];
      if (!sab || !reg || obj->fixedtype != DWG_TYPE_REGION)
        {
          free (sab);
          snprintf (msg, sizeof msg, "no REGION %u", i);
          break;
        }
      memcpy (sab, "ACIS BinaryFile", 15);
      for (BITCODE_BL j = 15; j < sizes[i]; j++)
        sab[j] = (unsigned char)((j * 131 + i * 7) & 0xff);
      if (dwg_obj_set_sab (obj, sab, sizes[i]))
        {
          free (sab);
          snprintf (msg, sizeof msg, "cannot set the SAB of REGION %u", i);
        }
    }
  if (!*msg)
    {
      dat.version = dat.from_version = R_2018;
      dat.opts = dwg.opts; // INDXF: header strings are TV, not TU
      error = dwg_encode (&dwg, &dat);
      if (error >= DWG_ERR_CRITICAL || !dat.chain)
        snprintf (msg, sizeof msg, "dwg_encode error 0x%x", error);
    }
  if (!*msg)
    {
      dat.size = dat.byte;
      dat.byte = 0;
      dat.bit = 0;
      dat.opts = 0; // binary DWG: strings are TU again
      error = dwg_decode (&dat, &dwg2);
      if (error >= DWG_ERR_CRITICAL)
        snprintf (msg, sizeof msg, "dwg_decode error 0x%x", error);
    }
  for (unsigned i = 0; !*msg && i < dwg.num_objects; i++)
    {
      const Dwg_Object *obj = &dwg.object[i];
      const Dwg_Object *o2;
      const unsigned char *d1, *d2;
      BITCODE_BL s1, s2;
      if (obj->fixedtype != DWG_TYPE_REGION
          || !dwg_obj_get_sab (obj, &d1, &s1))
        continue;
      num_regions++;
      o2 = dwg_resolve_handle (&dwg2, obj->handle.value);
      if (!o2 || !dwg_obj_get_sab (o2, &d2, &s2))
        snprintf (msg, sizeof msg, "REGION %lX has no SAB after the roundtrip",
                  (unsigned long)obj->handle.value);
      else if (s1 != s2 || memcmp (d1, d2, s1))
        snprintf (msg, sizeof msg, "REGION %lX SAB %u differs from %u",
                  (unsigned long)obj->handle.value, (unsigned)s2,
                  (unsigned)s1);
      else
        found++;
    }
  if (!*msg && (num_regions != 3 || found != 3))
    snprintf (msg, sizeof msg, "%u of 3 REGIONs, %u found", num_regions,
              found);
  dwg_free (&dwg);
  dwg_free (&dwg2);
  free (dat.chain);
  // dwg_free reset the loglevel
  loglevel = saved_loglevel;
  if (*msg)
    fail ("acds_sab: %s", msg);
  else
    ok ("acds_sab: SAB of 3 REGIONs, one in a blob, roundtripped via AcDs");
}

/* The AcDs data records which do not belong to an ACIS entity, i.e. the
   thumbnail PNG, are in the JSON AcDs.records, so that a JSON roundtrip
   keeps them. */
static void
acds_records_json_test (void)
{
  const char *src = "{\n"
                    " \"created_by\": \"libredwg test\",\n"
                    " \"FILEHEADER\": { \"version\": \"AC1032\" },\n"
                    " \"AcDs\": { \"records\": [\n"
                    "   { \"handle\": 34, \"schidx\": 0, \"size\": 8,\n"
                    "     \"data\": \"89504E470D0A1A0A\" },\n"
                    "   { \"handle\": 1000, \"schidx\": 1, \"size\": 3,\n"
                    "     \"data\": \"414243\" } ] },\n"
                    " \"OBJECTS\": [] }";
  const unsigned saved_loglevel = loglevel;
  char msg[160] = "";
  char *json = (char *)malloc (strlen (src) + 1);
  char *json2 = NULL;
  Dwg_Data dwg, dwg2;
  Bit_Chain dat, dat2;
  long len = 0;
  int error;

  memset (&dwg, 0, sizeof dwg);
  memset (&dwg2, 0, sizeof dwg2);
  memset (&dat, 0, sizeof dat);
  memset (&dat2, 0, sizeof dat2);
  if (!json)
    {
      fail ("acds_records_json: malloc failed");
      return;
    }
  strcpy (json, src);
  dat.chain = (unsigned char *)json;
  dat.size = strlen (json);
  error = dwg_read_json (&dat, &dwg);
  if (error >= DWG_ERR_CRITICAL)
    snprintf (msg, sizeof msg, "dwg_read_json error 0x%x", error);
  else if (dwg.acds.num_records != 2 || !dwg.acds.records
           || dwg.acds.records[0].handle != 34
           || dwg.acds.records[0].schidx != 0 || dwg.acds.records[0].size != 8
           || !dwg.acds.records[0].data
           || memcmp (dwg.acds.records[0].data, "\x89PNG\r\n\x1a\n", 8)
           || dwg.acds.records[1].handle != 1000
           || dwg.acds.records[1].size != 3 || !dwg.acds.records[1].data
           || memcmp (dwg.acds.records[1].data, "ABC", 3))
    snprintf (msg, sizeof msg, "records not read, %u",
              (unsigned)dwg.acds.num_records);
  if (!*msg)
    {
      dat2.fh = tmpfile ();
      dwg.header.version = dwg.header.from_version;
      dat2.version = dwg.header.version;
      dat2.from_version = dwg.header.from_version;
      if (!dat2.fh)
        snprintf (msg, sizeof msg, "no tmpfile");
      else
        {
          error = dwg_write_json_dat (&dat2, &dwg);
          if (error >= DWG_ERR_CRITICAL)
            snprintf (msg, sizeof msg, "dwg_write_json_dat error 0x%x", error);
          else
            {
              len = ftell (dat2.fh);
              rewind (dat2.fh);
              json2 = (char *)malloc (len + 1);
              if (!json2 || fread (json2, 1, len, dat2.fh) != (size_t)len)
                snprintf (msg, sizeof msg, "cannot read the JSON back");
              else
                json2[len] = '\0';
            }
          fclose (dat2.fh);
        }
    }
  if (!*msg)
    {
      memset (&dat, 0, sizeof dat);
      dat.chain = (unsigned char *)json2;
      dat.size = len;
      error = dwg_read_json (&dat, &dwg2);
      if (error >= DWG_ERR_CRITICAL)
        snprintf (msg, sizeof msg, "second dwg_read_json error 0x%x", error);
      else if (dwg2.acds.num_records != 2 || dwg2.acds.records[0].handle != 34
               || dwg2.acds.records[0].size != 8
               || memcmp (dwg2.acds.records[0].data, "\x89PNG\r\n\x1a\n", 8)
               || dwg2.acds.records[1].handle != 1000
               || dwg2.acds.records[1].schidx != 1
               || memcmp (dwg2.acds.records[1].data, "ABC", 3))
        snprintf (msg, sizeof msg, "records differ after the roundtrip");
    }
  dwg_free (&dwg);
  dwg_free (&dwg2);
  free (json);
  free (json2);
  loglevel = saved_loglevel;
  if (*msg)
    fail ("acds_records_json: %s", msg);
  else
    ok ("acds_records_json: the AcDs thumbnail records roundtrip via JSON");
}

/* Class strings can exceed the 16-bit short footer. */
static void
large_class_strings_test (void)
{
  Dwg_Data dwg = { 0 }, back = { 0 };
  Bit_Chain dat = { 0 };
  char name[80];
  char msg[128] = "";
  unsigned i;
  int error;

  dwg.header.version = dwg.header.from_version = R_2018;
  error = dwg_add_Document (&dwg, 0);
  for (i = 0; !error && i < 140; i++)
    {
      snprintf (name, sizeof name, "ROUNDTRIP_CLASS_%03u_01234567890123456789",
                i);
      if (dwg_add_class (&dwg, name, name, name, false) < 0)
        error = 1;
    }
  if (error)
    snprintf (msg, sizeof msg, "cannot construct classes");
  else
    {
      dat.version = dat.from_version = R_2018;
      dat.opts = dwg.opts;
      error = dwg_encode (&dwg, &dat);
      if (error >= DWG_ERR_CRITICAL || !dat.chain)
        snprintf (msg, sizeof msg, "encode error 0x%x", error);
    }
  if (!*msg)
    {
      dat.size = dat.byte;
      dat.byte = dat.bit = 0;
      dat.opts = 0;
      error = dwg_decode (&dat, &back);
      if (error >= DWG_ERR_CRITICAL || back.num_classes != dwg.num_classes
          || !back.dwg_class || !back.dwg_class[back.num_classes - 1].dxfname
          || strcmp (back.dwg_class[back.num_classes - 1].dxfname, name))
        snprintf (msg, sizeof msg, "class strings lost (error 0x%x)", error);
    }
  dwg_free (&dwg);
  dwg_free (&back);
  free (dat.chain);
  if (*msg)
    fail ("large_class_strings: %s", msg);
  else
    ok ("large_class_strings: extended class stream roundtrip");
}

/* The r2007 compression is the inverse of decompress_r2007, for data with
   long matches, short matches and only literals. */
static void
compress_r2007_tests (void)
{
  unsigned seed = 12345;
  int t, bad = 0, compressed = 0;

  for (t = 0; t < 120; t++)
    {
      const size_t n = 16 + (size_t)(t * 797) % 70000;
      unsigned char *src = (unsigned char *)malloc (n);
      unsigned char *dst = (unsigned char *)calloc (1, n + 16);
      unsigned char *out = (unsigned char *)malloc (n + n / 8 + 64);
      size_t i, clen = 0;

      for (i = 0; src && i < n; i++)
        {
          seed = seed * 1103515245U + 12345U;
          switch (t % 4)
            {
            case 0: // random
              src[i] = (unsigned char)(seed >> 16);
              break;
            case 1: // sparse
              src[i] = (i % 7 == 0) ? (unsigned char)(seed >> 16) : 0;
              break;
            case 2: // periodic
              src[i] = (unsigned char)"abcabcabcd"[i % 10];
              break;
            default: // some noise, else a copy of the byte 8 before
              src[i] = ((seed >> 16) % 5 == 0 || i < 8)
                           ? (unsigned char)(seed >> 8)
                           : src[i - 8];
              break;
            }
        }
      if (!src || !dst || !out
          || compress_r2007 (out, src, n, &clen) >= DWG_ERR_CRITICAL)
        bad++;
      else if (clen)
        {
          compressed++;
          if (clen >= n
              || decompress_r2007 (dst, (unsigned)n, out, (unsigned)clen, NULL)
              || memcmp (src, dst, n))
            bad++;
        }
      free (src);
      free (dst);
      free (out);
    }
  if (bad || !compressed)
    fail ("compress_r2007: %d of %d buffers do not roundtrip", bad,
          compressed);
  else
    ok ("compress_r2007: %d buffers roundtrip", compressed);
}

/* A r2007 DWG is written as such, with the RS coded pages, and read again. */
static void
encode_r2007_test (void)
{
  Dwg_Data dwg = { 0 }, back = { 0 };
  Bit_Chain dat = { 0 };
  char name[80];
  char msg[128] = "";
  unsigned i;
  int error;

  dwg.header.version = dwg.header.from_version = R_2007;
  error = dwg_add_Document (&dwg, 0);
  for (i = 0; !error && i < 40; i++)
    {
      snprintf (name, sizeof name, "ROUNDTRIP_R2007_CLASS_%03u", i);
      if (dwg_add_class (&dwg, name, name, name, false) < 0)
        error = 1;
    }
  if (!error)
    {
      /* BL in r2007, as in AutoCAD's files: a BS cannot hold these */
      dwg.dwg_class[0].dwg_version = 0x10021;
      dwg.dwg_class[0].maint_version = 427;
    }
  if (error)
    snprintf (msg, sizeof msg, "cannot construct the document");
  else
    {
      dat.version = dat.from_version = R_2007;
      dat.opts = dwg.opts;
      error = dwg_encode (&dwg, &dat);
      if (error >= DWG_ERR_CRITICAL || !dat.chain)
        snprintf (msg, sizeof msg, "encode error 0x%x", error);
      else if (memcmp (dat.chain, "AC1021", 6))
        snprintf (msg, sizeof msg, "no AC1021 magic");
    }
  if (!*msg)
    {
      dat.size = dat.byte;
      dat.byte = dat.bit = 0;
      dat.opts = 0;
      error = dwg_decode (&dat, &back);
      if (error >= DWG_ERR_CRITICAL || back.header.version != R_2007
          || back.num_classes != dwg.num_classes
          || back.num_objects != dwg.num_objects || !back.dwg_class
          || !back.dwg_class[back.num_classes - 1].dxfname
          || strcmp (back.dwg_class[back.num_classes - 1].dxfname, name))
        snprintf (msg, sizeof msg,
                  "not read back: error 0x%x, %u classes, %u objects", error,
                  (unsigned)back.num_classes, (unsigned)back.num_objects);
      else if (back.dwg_class[0].dwg_version != 0x10021
               || back.dwg_class[0].maint_version != 427)
        snprintf (msg, sizeof msg,
                  "class versions not read back: %u, %u",
                  (unsigned)back.dwg_class[0].dwg_version,
                  (unsigned)back.dwg_class[0].maint_version);
    }
  dwg_free (&dwg);
  dwg_free (&back);
  free (dat.chain);
  if (*msg)
    fail ("encode_r2007: %s", msg);
  else
    ok ("encode_r2007: the r2007 container is read back");
}

/* The r2004 container (r2004 and r2010+) as AutoCAD writes it. The rules
   are checked on AutoCAD's files of test-data and on the encoder's output of
   the same drawings, which is also read back. The file is read byte by byte,
   so the checks do not depend on the host's endianness. */

static uint32_t
le32_at (const unsigned char *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static unsigned
be16_at (const unsigned char *p)
{
  return ((unsigned)p[0] << 8) | p[1];
}

/* The magic sequence of the r2004 file header (ODA 4.1). */
static void
r2004_magic (unsigned char *magic)
{
  uint32_t rseed = 1;
  unsigned k;
  for (k = 0; k < 0x100; k++)
    {
      rseed = rseed * 0x343fd + 0x269ec3;
      magic[k] = (unsigned char)(rseed >> 16);
    }
}

/* A test-data file as it is. */
static int
read_test_data (const char *name, Bit_Chain *file)
{
  const char *srcdir = getenv ("top_srcdir");
  char path[1024];
  FILE *fp;
  long len;
  memset (file, 0, sizeof (Bit_Chain));
  if (!srcdir)
    srcdir = "../..";
  if (snprintf (path, sizeof (path), "%s/test/test-data/%s", srcdir, name)
      >= (int)sizeof (path))
    return 1;
  fp = fopen (path, "rb");
  if (!fp)
    return 1;
  if (fseek (fp, 0, SEEK_END) || (len = ftell (fp)) <= 0x100
      || fseek (fp, 0, SEEK_SET))
    {
      fclose (fp);
      return 1;
    }
  file->chain = (unsigned char *)malloc ((size_t)len);
  if (!file->chain || fread (file->chain, 1, (size_t)len, fp) != (size_t)len)
    {
      fclose (fp);
      free (file->chain);
      file->chain = NULL;
      return 1;
    }
  fclose (fp);
  file->size = (size_t)len;
  return 0;
}

/* The section of the given type, its pages decompressed at their offsets,
   as read_2004_compressed_section reads it. NULL if it has no pages. */
static unsigned char *
r2004_section (const Dwg_Data *dwg, const Bit_Chain *file,
               const Dwg_Section_Type type, size_t *sizep)
{
  const Dwg_Section_Info *info = NULL;
  unsigned char *buf;
  size_t alloc;
  BITCODE_BL i;
  for (i = 0; i < dwg->header.section_infohdr.num_desc; i++)
    if (dwg->header.section_info[i].fixedtype == type)
      {
        info = &dwg->header.section_info[i];
        break;
      }
  if (!info || !info->num_sections || !info->sections || info->size <= 0)
    return NULL;
  alloc = (size_t)info->num_sections * info->max_decomp_size;
  if (alloc < (size_t)info->size + info->max_decomp_size)
    alloc = (size_t)info->size + info->max_decomp_size;
  buf = (unsigned char *)calloc (alloc, 1);
  for (i = 0; buf && i < info->num_sections; i++)
    {
      const Dwg_Section *page = info->sections[i];
      uint32_t w[8];
      uint32_t mask;
      unsigned k;
      if (!page)
        continue;
      if (page->address + 32 > file->size)
        break;
      /* the page header, encrypted with its address (ODA 4.6) */
      mask = 0x4164536b ^ (uint32_t)page->address;
      for (k = 0; k < 8; k++)
        w[k] = le32_at (&file->chain[page->address + 4 * k]) ^ mask;
      if (w[0] != 0x4163043b || w[4] >= alloc)
        break;
      if (info->compressed == 2)
        {
          Bit_Chain src = { 0 }, dec = { 0 };
          src.chain = file->chain;
          src.byte = (size_t)page->address + 32;
          src.size = MIN (src.byte + w[2], file->size);
          dec.chain = buf;
          dec.size = alloc;
          dec.byte = w[4];
          if (decompress_R2004_section (&src, &dec) >= DWG_ERR_CRITICAL)
            {
              buf = dec.chain;
              break;
            }
          buf = dec.chain; // may be realloced
          alloc = MAX (alloc, dec.size);
        }
      else
        {
          if (w[4] + (size_t)w[3] > alloc
              || page->address + 32 + w[3] > file->size)
            break;
          memcpy (&buf[w[4]], &file->chain[page->address + 32], w[3]);
        }
    }
  if (!buf || i < info->num_sections)
    {
      free (buf);
      return NULL;
    }
  *sizep = (size_t)info->size;
  return buf;
}

/* Checks the container of one r2004+ file. The encoder's output (ours) has
   no gaps, so also no gap tree. Returns the number of problems, listed in
   msg. */
static int
r2004_container_check (const Bit_Chain *file, const int ours, char *msg,
                       const size_t msgsize)
{
  const unsigned level = loglevel;
  Dwg_Data dwg = { 0 };
  Bit_Chain dat = { 0 };
  unsigned char magic[0x100], hdr[0x6c];
  unsigned char *sec;
  size_t secsize, k;
  int bad = 0, error;

#define R2004_BAD(...)                                                        \
  do                                                                          \
    {                                                                         \
      const size_t len = strlen (msg);                                        \
      if (bad++ && len + 2 < msgsize)                                         \
        snprintf (&msg[len], msgsize - len, "; ");                            \
      if (strlen (msg) < msgsize)                                             \
        snprintf (&msg[strlen (msg)], msgsize - strlen (msg), __VA_ARGS__);   \
    }                                                                         \
  while (0)

  msg[0] = '\0';
  r2004_magic (magic);
  /* 0xEC..0xFF: the magic sequence as it is, not encrypted (ODA 4.1) */
  if (memcmp (&file->chain[0xEC], &magic[0xEC], 0x14))
    R2004_BAD ("bytes 0xEC..0xFF are not the magic sequence");
  for (k = 0; k < sizeof (hdr); k++)
    hdr[k] = file->chain[0x80 + k] ^ magic[k];
  if (memcmp (hdr, "AcFssFcAJMB", 12))
    R2004_BAD ("no AcFssFcAJMB header");
  /* the gap tree (root, lowermost left, right), numgaps, gap_array_size */
  if (ours
      && (le32_at (&hdr[0x18]) || le32_at (&hdr[0x1C]) || le32_at (&hdr[0x20])
          || le32_at (&hdr[0x3C]) || le32_at (&hdr[0x64])))
    R2004_BAD ("gaps: tree %d/%d/%d, %u gaps, gap array %u",
               (int)le32_at (&hdr[0x18]), (int)le32_at (&hdr[0x1C]),
               (int)le32_at (&hdr[0x20]), (unsigned)le32_at (&hdr[0x3C]),
               (unsigned)le32_at (&hdr[0x64]));

  dat.chain = (unsigned char *)malloc (file->size);
  if (!dat.chain)
    return 1;
  memcpy (dat.chain, file->chain, file->size);
  dat.size = file->size;
  loglevel = 0; // a whole drawing
  error = dwg_decode (&dat, &dwg);
  loglevel = level;
  if (error >= DWG_ERR_CRITICAL || !dwg.header.section_infohdr.num_desc)
    {
      R2004_BAD ("not read: error 0x%x", error);
      dwg_free (&dwg);
      free (dat.chain);
      return bad;
    }

  /* the empty section 0, first in the section map */
  if (dwg.header.section_info[0].name[0]
      || dwg.header.section_info[0].num_sections
      || dwg.header.section_info[0].size)
    R2004_BAD ("section map starts with %s, not the empty section",
               dwg.header.section_info[0].name);

  /* AcDb:Classes: the size counts the bytes after the size words, the
     r2010+ high size word holds the high 32 bits of the bit size, then the
     CRC, the end sentinel and 8 zero bytes */
  sec = r2004_section (&dwg, file, SECTION_CLASSES, &secsize);
  if (!sec)
    R2004_BAD ("no AcDb:Classes");
  else
    {
      const int hsize
          = (dwg.header.version >= R_2010 && dwg.header.maint_version > 3)
            || dwg.header.version >= R_2018;
      const size_t crc_pos
          = secsize < 24 ? 0 : (hsize ? 24 : 20) + (size_t)le32_at (&sec[16]);
      if (secsize < 24
          || memcmp (sec, dwg_sentinel (DWG_SENTINEL_CLASS_BEGIN), 16))
        R2004_BAD ("AcDb:Classes: no start sentinel");
      else
        {
          if (hsize && le32_at (&sec[20]))
            R2004_BAD ("AcDb:Classes: high size word %u",
                       (unsigned)le32_at (&sec[20]));
          if (crc_pos + 2 + 16 > secsize)
            R2004_BAD ("AcDb:Classes: size %u in a section of %u",
                       (unsigned)le32_at (&sec[16]), (unsigned)secsize);
          else if ((sec[crc_pos] | (sec[crc_pos + 1] << 8))
                       != bit_calc_CRC (0xC0C1, &sec[16], crc_pos - 16)
                   || memcmp (&sec[crc_pos + 2],
                              dwg_sentinel (DWG_SENTINEL_CLASS_END), 16))
            R2004_BAD ("AcDb:Classes: no CRC and end sentinel at the size");
          else if (crc_pos + 18 + 8 > secsize)
            R2004_BAD ("AcDb:Classes: no 8 zero bytes at the end");
          else
            for (k = crc_pos + 18; k < crc_pos + 26; k++)
              if (sec[k])
                {
                  R2004_BAD ("AcDb:Classes: no 8 zero bytes at the end");
                  break;
                }
        }
      free (sec);
    }

  /* AcDb:AcDbObjects starts with RL 0x0dca (ODA 20) */
  sec = r2004_section (&dwg, file, SECTION_OBJECTS, &secsize);
  if (!sec || secsize < 4 || le32_at (sec) != 0x0dca)
    R2004_BAD ("AcDb:AcDbObjects does not start with 0x0dca");
  free (sec);

  /* AcDb:Handles: pages of at most 2032 bytes (0x7F0) with the size, each
     followed by its CRC, then an empty page */
  sec = r2004_section (&dwg, file, SECTION_HANDLES, &secsize);
  if (!sec)
    R2004_BAD ("no AcDb:Handles");
  else
    {
      size_t pos = 0;
      unsigned n = 0, pages = 0;
      while (pos + 2 <= secsize && (n = be16_at (&sec[pos])) > 2)
        {
          if (n > 0x7F0 || pos + n + 2 > secsize)
            {
              R2004_BAD ("AcDb:Handles: page %u of %u bytes", pages, n);
              break;
            }
          if (be16_at (&sec[pos + n]) != bit_calc_CRC (0xC0C1, &sec[pos], n))
            {
              R2004_BAD ("AcDb:Handles: page %u CRC", pages);
              break;
            }
          pos += n + 2;
          pages++;
        }
      if (n != 2 && !bad)
        R2004_BAD ("AcDb:Handles: no empty last page");
      free (sec);
    }
#undef R2004_BAD
  dwg_free (&dwg);
  free (dat.chain);
  return bad;
}

/* The encoder's output of a test-data drawing, at the given version. */
static int
r2004_encode (const Bit_Chain *file, const Dwg_Version_Type version,
              Bit_Chain *out, BITCODE_BL *num_objects)
{
  const unsigned level = loglevel;
  Dwg_Data dwg = { 0 };
  Bit_Chain dat = { 0 };
  int error;
  dat.chain = (unsigned char *)malloc (file->size);
  if (!dat.chain)
    return DWG_ERR_OUTOFMEM;
  memcpy (dat.chain, file->chain, file->size);
  dat.size = file->size;
  loglevel = 0; // a whole drawing
  error = dwg_decode (&dat, &dwg);
  free (dat.chain);
  if (error >= DWG_ERR_CRITICAL)
    {
      dwg_free (&dwg);
      loglevel = level;
      return error;
    }
  memset (out, 0, sizeof (Bit_Chain));
  dwg.header.version = version;
  out->version = version;
  out->from_version = dwg.header.from_version;
  out->codepage = dwg.header.codepage;
  out->opts = dwg.opts;
  error = dwg_encode (&dwg, out);
  *num_objects = dwg.num_objects; // a downconversion may add some
  out->size = out->byte;
  out->byte = out->bit = 0;
  dwg_free (&dwg);
  loglevel = level;
  return error;
}

static void
r2004_container_test (void)
{
  /* AutoCAD's files: AutoCAD 2018's saves as r2004-r2018, and AutoCAD
     2013's 2013/Arc.dwg, which has gaps and a gap tree. */
  static const char *const acad[]
      = { "example_2004.dwg", "example_2010.dwg", "example_2013.dwg",
          "example_2018.dwg", "2013/Arc.dwg" };
  /* the encoder's output: the same version, and r2018 saved to an older
     release */
  static const struct
  {
    const char *name;
    Dwg_Version_Type version;
  } enc[] = { { "example_2004.dwg", R_2004 }, { "example_2010.dwg", R_2010 },
              { "example_2013.dwg", R_2013 }, { "example_2018.dwg", R_2018 },
              { "2013/Arc.dwg", R_2013 },     { "example_2018.dwg", R_2004 },
              { "example_2018.dwg", R_2010 } };
  char msg[600];
  unsigned i;

  loglevel = is_make_silent () ? 0 : 3; // dwg_encode resets it
  for (i = 0; i < ARRAY_SIZE (acad); i++)
    {
      Bit_Chain file;
      if (read_test_data (acad[i], &file))
        {
          fail ("r2004 container: %s not found", acad[i]);
          continue;
        }
      if (r2004_container_check (&file, 0, msg, sizeof (msg)))
        fail ("r2004 container of AutoCAD's %s: %s", acad[i], msg);
      else
        ok ("r2004 container of AutoCAD's %s", acad[i]);
      free (file.chain);
    }

  for (i = 0; i < ARRAY_SIZE (enc); i++)
    {
      Bit_Chain file, out;
      BITCODE_BL num_objects = 0;
      Dwg_Data back = { 0 };
      int error;
      if (read_test_data (enc[i].name, &file))
        {
          fail ("r2004 container: %s not found", enc[i].name);
          continue;
        }
      error = r2004_encode (&file, enc[i].version, &out, &num_objects);
      free (file.chain);
      if (error >= DWG_ERR_CRITICAL || !out.chain)
        {
          fail ("r2004 container: %s as %s: encode error 0x%x", enc[i].name,
                dwg_version_type (enc[i].version), error);
          free (out.chain);
          continue;
        }
      if (r2004_container_check (&out, 1, msg, sizeof (msg)))
        fail ("r2004 container: %s as %s: %s", enc[i].name,
              dwg_version_type (enc[i].version), msg);
      else
        {
          /* read back: the object map offsets count from the start of
             AcDb:AcDbObjects, after the 0x0dca */
          loglevel = 0; // a whole drawing
          error = dwg_decode (&out, &back);
          loglevel = is_make_silent () ? 0 : 3;
          if (error >= DWG_ERR_CRITICAL || back.num_objects != num_objects)
            fail ("r2004 container: %s as %s: read back %u of %u objects "
                  "(error 0x%x)",
                  enc[i].name, dwg_version_type (enc[i].version),
                  (unsigned)back.num_objects, (unsigned)num_objects, error);
          else
            ok ("r2004 container: %s as %s, %u objects read back", enc[i].name,
                dwg_version_type (enc[i].version), (unsigned)num_objects);
          dwg_free (&back);
        }
      free (out.chain);
    }
}

/* maint_version (0x12) and app_maint_version (0x17): 0xff in AutoCAD LT
   2027's save to an older release (its r2007 fixture), and in the encoder's
   r2004-r2013 output of an r2018 drawing, but not in its r2018 output. */
static void
r2004_maint_version_test (void)
{
  static const Dwg_Version_Type older[] = { R_2004, R_2010, R_2013 };
  Bit_Chain file, out;
  BITCODE_BL num_objects;
  unsigned i;
  int error;

  loglevel = is_make_silent () ? 0 : 3; // dwg_encode resets it
  if (read_test_data ("2007/block_acad2027.dwg", &file))
    fail ("maint_version: 2007/block_acad2027.dwg not found");
  else
    {
      if (file.chain[0x12] == 0xff && file.chain[0x17] == 0xff)
        ok ("maint_version: AutoCAD LT 2027's r2007 save has 0xff");
      else
        fail ("maint_version: AutoCAD LT 2027's r2007 save has %u, %u",
              file.chain[0x12], file.chain[0x17]);
      free (file.chain);
    }
  if (read_test_data ("example_2018.dwg", &file))
    {
      fail ("maint_version: example_2018.dwg not found");
      return;
    }
  for (i = 0; i < ARRAY_SIZE (older); i++)
    {
      error = r2004_encode (&file, older[i], &out, &num_objects);
      if (error >= DWG_ERR_CRITICAL || out.size < 0x18)
        fail ("maint_version: r2018 as %s: encode error 0x%x",
              dwg_version_type (older[i]), error);
      else if (out.chain[0x12] != 0xff || out.chain[0x17] != 0xff)
        fail ("maint_version: r2018 as %s: %u, %u",
              dwg_version_type (older[i]), out.chain[0x12], out.chain[0x17]);
      else
        ok ("maint_version: r2018 as %s: 0xff", dwg_version_type (older[i]));
      free (out.chain);
    }
  error = r2004_encode (&file, R_2018, &out, &num_objects);
  if (error >= DWG_ERR_CRITICAL || out.size < 0x18)
    fail ("maint_version: r2018 as r2018: encode error 0x%x", error);
  else if (out.chain[0x12] == 0xff || out.chain[0x17] == 0xff)
    fail ("maint_version: r2018 as r2018: 0xff");
  else
    ok ("maint_version: r2018 as r2018: %u, not 0xff", out.chain[0x12]);
  free (out.chain);
  free (file.chain);
}

/* A test-data drawing, decoded. */
static int
decode_test_data (const char *name, Dwg_Data *dwg)
{
  const unsigned level = loglevel;
  Bit_Chain file;
  int error;
  memset (dwg, 0, sizeof (Dwg_Data));
  if (read_test_data (name, &file))
    return DWG_ERR_IOERROR;
  loglevel = 0; // a whole drawing
  error = dwg_decode (&file, dwg);
  loglevel = level;
  free (file.chain);
  return error;
}

/* The encoder's output of a test-data drawing at the given version, read
   back. */
static int
encode_test_data (const char *name, const Dwg_Version_Type version,
                  Dwg_Data *back)
{
  const unsigned level = loglevel;
  Bit_Chain file, out;
  BITCODE_BL num_objects;
  int error;
  memset (back, 0, sizeof (Dwg_Data));
  if (read_test_data (name, &file))
    return DWG_ERR_IOERROR;
  error = r2004_encode (&file, version, &out, &num_objects);
  free (file.chain);
  if (error >= DWG_ERR_CRITICAL || !out.chain)
    {
      free (out.chain);
      return error ? error : DWG_ERR_INTERNALERROR;
    }
  loglevel = 0; // a whole drawing
  error = dwg_decode (&out, back);
  loglevel = level;
  free (out.chain);
  return error;
}

/* Whether an EED item is the string s. */
static int
eed_is_string (const Dwg_Data *dwg, const Dwg_Eed *eed, const char *s)
{
  const size_t len = strlen (s);
  size_t i;
  if (!eed->data || eed->data->code != 0)
    return 0;
  if (dwg->header.from_version >= R_2007)
    {
      if (eed->data->u.eed_0_r2007.length != len)
        return 0;
      for (i = 0; i < len; i++)
        if (eed->data->u.eed_0_r2007.string[i] != (DWGCHAR)s[i])
          return 0;
      return 1;
    }
  return eed->data->u.eed_0.length == len
         && memcmp (eed->data->u.eed_0.string, s, len) == 0;
}

/* The index of the object's EED string item s, or -1. */
static int
eed_find_string (const Dwg_Data *dwg, const Dwg_Object *obj, const char *s)
{
  BITCODE_BL i;
  if (obj->supertype != DWG_SUPERTYPE_OBJECT || !obj->tio.object)
    return -1;
  for (i = 0; i < obj->tio.object->num_eed; i++)
    if (eed_is_string (dwg, &obj->tio.object->eed[i], s))
      return (int)i;
  return -1;
}

/* What the older-release checks count in a drawing. */
typedef struct
{
  unsigned saved_by;      // VISUALSTYLE and SUN with AcDbSavedByObjectVersion
  unsigned assoc_version; // ASSOCACTION class_version
  unsigned pab_deps;      // owned params in the aligned dim action body
  unsigned layer_mat;     // LAYERs whose material is a MATERIAL
  unsigned dimstyle_eed;  // DIMSTYLEs with EED
  unsigned layer_star;    // LAYERs named with a leading '*'
  unsigned vx;            // VX_TABLE_RECORDs
  unsigned vx_vports;     // the sum of their VIEWPORT handles
} older_counts;

static void
older_release_count (Dwg_Data *dwg, older_counts *c)
{
  BITCODE_BL i;
  memset (c, 0, sizeof (older_counts));
  for (i = 0; i < dwg->num_objects; i++)
    {
      Dwg_Object *obj = &dwg->object[i];
      switch (obj->fixedtype)
        {
        case DWG_TYPE_VISUALSTYLE:
        case DWG_TYPE_SUN:
          if (eed_find_string (dwg, obj, "AcDbSavedByObjectVersion") >= 0)
            c->saved_by++;
          break;
        case DWG_TYPE_ASSOCACTION:
          c->assoc_version = obj->tio.object->tio.ASSOCACTION->class_version;
          break;
        case DWG_TYPE_ASSOCALIGNEDDIMACTIONBODY:
          c->pab_deps
              = obj->tio.object->tio.ASSOCALIGNEDDIMACTIONBODY->pab.num_deps;
          break;
        case DWG_TYPE_LAYER:
          {
            Dwg_Object_LAYER *layer = obj->tio.object->tio.LAYER;
            Dwg_Object *mat = layer->material
                                  ? dwg_ref_object (dwg, layer->material)
                                  : NULL;
            if (mat && mat->fixedtype == DWG_TYPE_MATERIAL)
              c->layer_mat++;
            if (layer->name
                && (dwg->header.version >= R_2007
                        ? ((BITCODE_TU)(void *)layer->name)[0] == '*'
                        : layer->name[0] == '*'))
              c->layer_star++;
          }
          break;
        case DWG_TYPE_DIMSTYLE:
          if (obj->tio.object->num_eed)
            c->dimstyle_eed++;
          break;
        case DWG_TYPE_VX_TABLE_RECORD:
          c->vx++;
          if (obj->tio.object->tio.VX_TABLE_RECORD->viewport)
            c->vx_vports += (unsigned)obj->tio.object->tio.VX_TABLE_RECORD
                                ->viewport->absolute_ref;
          break;
        default:
          break;
        }
    }
}

/* The class names of a pre-r2007 drawing against AutoCAD's, by DXF name.
   Returns the number of classes that differ. */
static unsigned
older_release_classes (const Dwg_Data *acad, const Dwg_Data *ours, char *msg,
                       const size_t msgsize)
{
  unsigned bad = 0;
  BITCODE_BS i, j;
  msg[0] = '\0';
  for (i = 0; i < ours->num_classes; i++)
    {
      const Dwg_Class *k = &ours->dwg_class[i];
      for (j = 0; j < acad->num_classes; j++)
        {
          const Dwg_Class *a = &acad->dwg_class[j];
          if (!k->dxfname || !a->dxfname || strNE (k->dxfname, a->dxfname))
            continue;
          if (!k->appname || !k->cppname || strNE (k->appname, a->appname)
              || strNE (k->cppname, a->cppname))
            {
              if (!bad++)
                snprintf (msg, msgsize, "%s: \"%s\" \"%s\"", k->dxfname,
                          k->appname ? k->appname : "",
                          k->cppname ? k->cppname : "");
            }
          break;
        }
    }
  return bad;
}

/* example_2018.dwg saved to an older release by the encoder, against
   AutoCAD 2018's saves of the same drawing to that release. */
static void
older_release_test (void)
{
  static const struct
  {
    const char *acad;
    Dwg_Version_Type version;
  } t[] = { { "example_2000.dwg", R_2000 },
            { "example_2004.dwg", R_2004 },
            { "example_2007.dwg", R_2007 },
            { "example_2010.dwg", R_2010 },
            { "example_2013.dwg", R_2013 } };
  unsigned i;

  loglevel = is_make_silent () ? 0 : 3; // dwg_encode resets it
  for (i = 0; i < ARRAY_SIZE (t); i++)
    {
      Dwg_Data acad, ours;
      older_counts a, o;
      const char *v = dwg_version_type (t[i].version);
      int error = decode_test_data (t[i].acad, &acad);
      if (error >= DWG_ERR_CRITICAL)
        {
          fail ("older release: %s not read (0x%x)", t[i].acad, error);
          dwg_free (&acad);
          continue;
        }
      error = encode_test_data ("example_2018.dwg", t[i].version, &ours);
      if (error >= DWG_ERR_CRITICAL)
        {
          fail ("older release: example_2018.dwg as %s not read back (0x%x)",
                v, error);
          dwg_free (&acad);
          dwg_free (&ours);
          continue;
        }
      older_release_count (&acad, &a);
      older_release_count (&ours, &o);

      /* their format changed in r2010: the ACAD EED with the saved-by
         version */
      if (o.saved_by == a.saved_by)
        ok ("older release %s: %u VISUALSTYLE/SUN saved by version", v,
            o.saved_by);
      else
        fail ("older release %s: %u VISUALSTYLE/SUN saved by version, "
              "AutoCAD %u",
              v, o.saved_by, a.saved_by);
      /* r2010 and older: ASSOCACTION class_version 1, its params in the
         action body */
      if (o.assoc_version == a.assoc_version && o.pab_deps == a.pab_deps)
        ok ("older release %s: ASSOCACTION class_version %u, %u action "
            "body param(s)",
            v, o.assoc_version, o.pab_deps);
      else
        fail ("older release %s: ASSOCACTION class_version %u, %u action "
              "body param(s), AutoCAD %u, %u",
              v, o.assoc_version, o.pab_deps, a.assoc_version, a.pab_deps);
      if (o.layer_mat == a.layer_mat)
        ok ("older release %s: %u LAYER material(s)", v, o.layer_mat);
      else
        fail ("older release %s: %u LAYER material(s), AutoCAD %u", v,
              o.layer_mat, a.layer_mat);
      if (o.dimstyle_eed == a.dimstyle_eed)
        ok ("older release %s: %u DIMSTYLE(s) with EED", v, o.dimstyle_eed);
      else
        fail ("older release %s: %u DIMSTYLE(s) with EED, AutoCAD %u", v,
              o.dimstyle_eed, a.dimstyle_eed);
      /* *ADSK_SYSTEM_LIGHTS without its '*' before r2007 */
      if (o.layer_star == a.layer_star)
        ok ("older release %s: %u LAYER name(s) with a leading '*'", v,
            o.layer_star);
      else
        fail ("older release %s: %u LAYER name(s) with a leading '*', "
              "AutoCAD %u",
              v, o.layer_star, a.layer_star);
      if (t[i].version == R_2000)
        {
          /* an empty VX record, then one per VIEWPORT entity */
          if (o.vx == a.vx && o.vx_vports == a.vx_vports)
            ok ("older release %s: %u VX_TABLE_RECORDs", v, o.vx);
          else
            fail ("older release %s: %u VX_TABLE_RECORDs (VIEWPORTs %u), "
                  "AutoCAD %u (%u)",
                  v, o.vx, o.vx_vports, a.vx, a.vx_vports);
        }
      if (t[i].version < R_2007)
        {
          /* TV strings from the r2018 TU strings. AutoCAD keeps the
             FINGERPRINTGUID; the VERSIONGUID can change with each save. */
          char msg[200];
          const char *fg = ours.header_vars.FINGERPRINTGUID;
          const char *vg = ours.header_vars.VERSIONGUID;
          if (fg && vg && acad.header_vars.FINGERPRINTGUID
              && strEQ (fg, acad.header_vars.FINGERPRINTGUID)
              && strlen (vg) == 38 && vg[0] == '{' && vg[37] == '}')
            ok ("older release %s: FINGERPRINTGUID and VERSIONGUID", v);
          else
            fail ("older release %s: FINGERPRINTGUID \"%s\", VERSIONGUID "
                  "\"%s\"",
                  v, fg ? fg : "", vg ? vg : "");
          if (older_release_classes (&acad, &ours, msg, sizeof (msg)))
            fail ("older release %s: class names %s", v, msg);
          else
            ok ("older release %s: class names", v);
        }
      dwg_free (&acad);
      dwg_free (&ours);
    }
}

/* An r2000 PROXY_OBJECT from an r2004+ drawing: the ACAD EED 1000
   "AcRTDxfName", 1000 <the class's DXF name>, as AutoCAD writes it. */
static void
older_release_proxy_test (void)
{
  Dwg_Data ours;
  BITCODE_BL i;
  unsigned proxies = 0, named = 0;
  int error;

  loglevel = is_make_silent () ? 0 : 3; // dwg_encode resets it
  error = encode_test_data ("2018/Constraints.dwg", R_2000, &ours);
  if (error >= DWG_ERR_CRITICAL)
    {
      fail ("older release proxy: 2018/Constraints.dwg as r2000 not read "
            "back (0x%x)",
            error);
      dwg_free (&ours);
      return;
    }
  for (i = 0; i < ours.num_objects; i++)
    {
      const Dwg_Object *obj = &ours.object[i];
      const Dwg_Object_PROXY_OBJECT *proxy;
      const Dwg_Class *klass;
      int k;
      if (obj->fixedtype != DWG_TYPE_PROXY_OBJECT)
        continue;
      proxies++;
      proxy = obj->tio.object->tio.PROXY_OBJECT;
      klass
          = proxy->class_id >= 500 && proxy->class_id - 500 < ours.num_classes
                ? &ours.dwg_class[proxy->class_id - 500]
                : NULL;
      k = eed_find_string (&ours, obj, "AcRTDxfName");
      if (klass && klass->dxfname && k >= 0
          && (BITCODE_BL)k + 1 < obj->tio.object->num_eed
          && eed_is_string (&ours, &obj->tio.object->eed[k + 1],
                            klass->dxfname))
        named++;
    }
  if (proxies && named == proxies)
    ok ("older release proxy: %u r2000 PROXY_OBJECTs with their DXF name",
        proxies);
  else
    fail ("older release proxy: %u of %u r2000 PROXY_OBJECTs with their "
          "DXF name",
          named, proxies);
  dwg_free (&ours);
}

/* In r2007+ the header has no unknown_54 .. unknown_57: their bits are the
   start of the string stream. */
static void
header_unknown_54_test (void)
{
  static const char *const acad[] = { "example_2007.dwg", "example_2010.dwg",
                                      "example_2013.dwg", "example_2018.dwg" };
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (acad); i++)
    {
      Dwg_Data dwg;
      const int error = decode_test_data (acad[i], &dwg);
      const Dwg_Header_Variables *h = &dwg.header_vars;
      if (error >= DWG_ERR_CRITICAL)
        fail ("header: %s not read (0x%x)", acad[i], error);
      else if (h->unknown_54 || h->unknown_55 || h->unknown_56
               || h->unknown_57)
        fail ("header: %s has unknown_54 .. 57 %u %u %u %u", acad[i],
              h->unknown_54, h->unknown_55, h->unknown_56, h->unknown_57);
      else
        ok ("header: %s has no unknown_54 .. 57", acad[i]);
      dwg_free (&dwg);
    }
}

/* dwg_strip_mtext: the plain text of AutoCAD's MTEXT, for a single-line
   ATTRIB/ATTDEF in an older release and the r12 DXF TEXT. */
static void
strip_mtext_test (void)
{
  static const char *const t[][2] = {
    { "{\\fArial|b0|i0|c0|p34;Hello}\\PWorld", "Hello World" },
    { "a~b\\Nc", "a b c" },
    { "\\{x\\}\\\\", "{x}\\" },
    { "caf\\U+00E9", "caf\xc3\xa9" },
    { "\\S1^2; \\S3#4;", "1/2 3/4" },
    { "\\LUnder\\l \\H2.5x;\\C1;Big", "Under Big" },
    { "\\M+N8140x", "?x" },
    { "", "" },
  };
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (t); i++)
    {
      char *s = dwg_strip_mtext (t[i][0]);
      if (s && strEQ (s, t[i][1]))
        ok ("dwg_strip_mtext \"%s\"", t[i][0]);
      else
        fail ("dwg_strip_mtext \"%s\": \"%s\", not \"%s\"", t[i][0],
              s ? s : "(null)", t[i][1]);
      free (s);
    }
}

int
main (int argc, char const *argv[])
{
  Dwg_Data dwg;
  loglevel = is_make_silent () ? 0 : 3;
  dwg.header.version = R_2000;

  test_section_find (&dwg);
  test_section_move_top (&dwg);
  // loglevel = is_make_silent () ? 0 : 3;
  test_section_remove (&dwg);
  test_section_move_before (&dwg);

  compress_R2004_section_tests ();
  common_entity_handle_data_double_free_test ();
  remove_NOD_item_null_itemhandles_test ();
  in_json_section_counts_test ();
  in_json_object_counts_test ();
  in_json_subclass_fixed_elems_test ();
  in_json_mline_lines_test ();
  in_json_dotted_array_test ();
  dxf_as_acad_test ();
  dxf_r12_test ();
  in_dxf_empty_xdata_test ();
  downconvert_TABLE_test ();
  downconvert_TABLESTYLE_test ();
  downconvert_TABLESTYLE_custom_test ();
  acds_sab_test ();
  acds_records_json_test ();
  large_class_strings_test ();
  compress_r2007_tests ();
  encode_r2007_test ();
  r2004_container_test ();
  r2004_maint_version_test ();
  older_release_test ();
  older_release_proxy_test ();
  header_unknown_54_test ();
  strip_mtext_test ();
  return failed;
}

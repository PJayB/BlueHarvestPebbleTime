#pragma once

// ------------------------- impbg -------------------------
#define PREZR_IMPBG_CHECKSUM 0x7033739F

typedef enum prezr_pack_impbg_e {
  PREZR_IMPBG_IMPBG, // 144x168 Bit1Palettized
  PREZR_IMPBG_COUNT
} prezr_pack_impbg_t;

#if defined(PREZR_IMPORT) || defined(PREZR_IMPORT_IMPBG_PACK)
prezr_pack_t prezr_impbg = { NULL, 0, NULL };
void prezr_load_impbg() {
  int r = prezr_init(&prezr_impbg, RESOURCE_ID_PREZR_IMPBG_PACK);
  if (r != PREZR_OK)
    APP_LOG(APP_LOG_LEVEL_ERROR, "PRezr package 'impbg' failed with code %d", r);
  if (prezr_impbg.numResources != PREZR_IMPBG_COUNT)
    APP_LOG(APP_LOG_LEVEL_ERROR, "PRezr package 'impbg' resource count mismatch");
}
void prezr_unload_impbg() {
  prezr_destroy(&prezr_impbg);
}
#else
extern prezr_pack_t prezr_impbg;
extern void prezr_load_impbg();
extern void prezr_unload_impbg();
#endif // PREZR_IMPORT

// ------------------------- mandbg -------------------------
#define PREZR_MANDBG_CHECKSUM 0x70387D8D

typedef enum prezr_pack_mandbg_e {
  PREZR_MANDBG_MANDBG, // 144x168 Bit1Palettized
  PREZR_MANDBG_COUNT
} prezr_pack_mandbg_t;

#if defined(PREZR_IMPORT) || defined(PREZR_IMPORT_MANDBG_PACK)
prezr_pack_t prezr_mandbg = { NULL, 0, NULL };
void prezr_load_mandbg() {
  int r = prezr_init(&prezr_mandbg, RESOURCE_ID_PREZR_MANDBG_PACK);
  if (r != PREZR_OK)
    APP_LOG(APP_LOG_LEVEL_ERROR, "PRezr package 'mandbg' failed with code %d", r);
  if (prezr_mandbg.numResources != PREZR_MANDBG_COUNT)
    APP_LOG(APP_LOG_LEVEL_ERROR, "PRezr package 'mandbg' resource count mismatch");
}
void prezr_unload_mandbg() {
  prezr_destroy(&prezr_mandbg);
}
#else
extern prezr_pack_t prezr_mandbg;
extern void prezr_load_mandbg();
extern void prezr_unload_mandbg();
#endif // PREZR_IMPORT

// ------------------------- rebelbg -------------------------
#define PREZR_REBELBG_CHECKSUM 0x703CC430

typedef enum prezr_pack_rebelbg_e {
  PREZR_REBELBG_REBELBG, // 144x168 Bit1Palettized
  PREZR_REBELBG_COUNT
} prezr_pack_rebelbg_t;

#if defined(PREZR_IMPORT) || defined(PREZR_IMPORT_REBELBG_PACK)
prezr_pack_t prezr_rebelbg = { NULL, 0, NULL };
void prezr_load_rebelbg() {
  int r = prezr_init(&prezr_rebelbg, RESOURCE_ID_PREZR_REBELBG_PACK);
  if (r != PREZR_OK)
    APP_LOG(APP_LOG_LEVEL_ERROR, "PRezr package 'rebelbg' failed with code %d", r);
  if (prezr_rebelbg.numResources != PREZR_REBELBG_COUNT)
    APP_LOG(APP_LOG_LEVEL_ERROR, "PRezr package 'rebelbg' resource count mismatch");
}
void prezr_unload_rebelbg() {
  prezr_destroy(&prezr_rebelbg);
}
#else
extern prezr_pack_t prezr_rebelbg;
extern void prezr_load_rebelbg();
extern void prezr_unload_rebelbg();
#endif // PREZR_IMPORT


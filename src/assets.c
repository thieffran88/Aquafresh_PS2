/* Gerado por tools/gen_assets.py - nao edite. */
#include "assets.h"

extern const unsigned char asset_bg_play[];
extern const unsigned char asset_bg_title[];
extern const unsigned char asset_tiles[];
extern const unsigned char asset_deco[];
extern const unsigned char asset_font[];

const AtlasDef atlases[NUM_ATLASES] = {
    { 640, 448, asset_bg_play },
    { 640, 448, asset_bg_title },
    { 512, 384, asset_tiles },
    { 512, 256, asset_deco },
    { 512, 512, asset_font },
};

const SpriteDef sprite_defs[NUM_SPRITES] = {
    { ATLAS_TILES, 2, 2, 64, 56 }, /* TILE_00 */
    { ATLAS_TILES, 70, 2, 64, 56 }, /* TILE_01 */
    { ATLAS_TILES, 138, 2, 64, 56 }, /* TILE_02 */
    { ATLAS_TILES, 206, 2, 64, 56 }, /* TILE_03 */
    { ATLAS_TILES, 274, 2, 64, 56 }, /* TILE_04 */
    { ATLAS_TILES, 342, 2, 64, 56 }, /* TILE_05 */
    { ATLAS_TILES, 410, 2, 64, 56 }, /* TILE_06 */
    { ATLAS_TILES, 2, 62, 64, 56 }, /* TILE_07 */
    { ATLAS_TILES, 70, 62, 64, 56 }, /* TILE_08 */
    { ATLAS_TILES, 138, 62, 64, 56 }, /* TILE_09 */
    { ATLAS_TILES, 206, 62, 64, 56 }, /* TILE_10 */
    { ATLAS_TILES, 274, 62, 64, 56 }, /* TILE_11 */
    { ATLAS_TILES, 342, 62, 64, 56 }, /* TILE_12 */
    { ATLAS_TILES, 410, 62, 64, 56 }, /* TILE_13 */
    { ATLAS_TILES, 2, 122, 64, 56 }, /* TILE_14 */
    { ATLAS_TILES, 70, 122, 64, 56 }, /* TILE_15 */
    { ATLAS_TILES, 138, 122, 64, 56 }, /* TILE_16 */
    { ATLAS_TILES, 206, 122, 64, 56 }, /* TILE_17 */
    { ATLAS_TILES, 274, 122, 64, 56 }, /* TILE_18 */
    { ATLAS_TILES, 342, 122, 64, 56 }, /* TILE_19 */
    { ATLAS_TILES, 410, 122, 64, 56 }, /* TILE_20 */
    { ATLAS_TILES, 2, 182, 64, 56 }, /* TILE_21 */
    { ATLAS_TILES, 70, 182, 64, 56 }, /* TILE_22 */
    { ATLAS_TILES, 138, 182, 64, 56 }, /* TILE_23 */
    { ATLAS_TILES, 206, 182, 64, 56 }, /* TILE_24 */
    { ATLAS_TILES, 274, 182, 64, 56 }, /* TILE_25 */
    { ATLAS_TILES, 342, 182, 64, 56 }, /* TILE_26 */
    { ATLAS_TILES, 410, 182, 64, 56 }, /* TILE_27 */
    { ATLAS_TILES, 2, 242, 64, 56 }, /* TILE_28 */
    { ATLAS_TILES, 70, 242, 64, 56 }, /* TILE_29 */
    { ATLAS_TILES, 138, 242, 64, 56 }, /* TILE_30 */
    { ATLAS_TILES, 206, 242, 64, 56 }, /* TILE_31 */
    { ATLAS_TILES, 274, 242, 64, 56 }, /* TILE_32 */
    { ATLAS_TILES, 342, 242, 64, 56 }, /* TILE_33 */
    { ATLAS_TILES, 410, 242, 64, 56 }, /* TILE_34 */
    { ATLAS_TILES, 2, 302, 64, 56 }, /* TILE_35 */
    { ATLAS_DECO, 2, 2, 108, 84 }, /* TURTLE */
    { ATLAS_DECO, 113, 2, 94, 76 }, /* PUFFER */
    { ATLAS_DECO, 210, 2, 87, 76 }, /* STARFISH */
    { ATLAS_DECO, 300, 2, 62, 72 }, /* WEED */
    { ATLAS_DECO, 365, 2, 64, 72 }, /* CORAL_PINK */
    { ATLAS_DECO, 432, 2, 64, 72 }, /* CORAL_ORANGE */
    { ATLAS_DECO, 2, 89, 60, 72 }, /* CORAL_PURPLE */
    { ATLAS_DECO, 65, 89, 98, 64 }, /* CRAB */
    { ATLAS_DECO, 166, 89, 78, 56 }, /* CLAM */
    { ATLAS_DECO, 247, 89, 104, 56 }, /* ROCKS */
    { ATLAS_DECO, 354, 89, 64, 56 }, /* SLOT_DARK */
    { ATLAS_DECO, 421, 89, 64, 56 }, /* SLOT_LIGHT */
    { ATLAS_DECO, 2, 164, 72, 54 }, /* CORAL_TUBES */
    { ATLAS_DECO, 77, 164, 73, 50 }, /* CORAL_BALLS */
    { ATLAS_DECO, 153, 164, 79, 48 }, /* CORAL_GREEN */
    { ATLAS_DECO, 235, 164, 65, 40 }, /* SHELL */
    { ATLAS_DECO, 303, 164, 142, 40 }, /* SAND */
    { ATLAS_DECO, 448, 164, 34, 32 }, /* BUBBLE */
    { ATLAS_FONT, 2, 2, 50, 40 }, /* GLYPH_41 */
    { ATLAS_FONT, 55, 2, 38, 40 }, /* GLYPH_42 */
    { ATLAS_FONT, 96, 2, 40, 40 }, /* GLYPH_43 */
    { ATLAS_FONT, 139, 2, 41, 40 }, /* GLYPH_44 */
    { ATLAS_FONT, 183, 2, 39, 40 }, /* GLYPH_45 */
    { ATLAS_FONT, 225, 2, 39, 40 }, /* GLYPH_46 */
    { ATLAS_FONT, 267, 2, 38, 40 }, /* GLYPH_47 */
    { ATLAS_FONT, 308, 2, 42, 40 }, /* GLYPH_48 */
    { ATLAS_FONT, 353, 2, 26, 40 }, /* GLYPH_49 */
    { ATLAS_FONT, 382, 2, 36, 40 }, /* GLYPH_4A */
    { ATLAS_FONT, 421, 2, 46, 40 }, /* GLYPH_4B */
    { ATLAS_FONT, 470, 2, 36, 40 }, /* GLYPH_4C */
    { ATLAS_FONT, 2, 45, 50, 40 }, /* GLYPH_4D */
    { ATLAS_FONT, 55, 45, 44, 40 }, /* GLYPH_4E */
    { ATLAS_FONT, 102, 45, 39, 40 }, /* GLYPH_4F */
    { ATLAS_FONT, 144, 45, 38, 40 }, /* GLYPH_50 */
    { ATLAS_FONT, 185, 45, 39, 40 }, /* GLYPH_51 */
    { ATLAS_FONT, 227, 45, 38, 40 }, /* GLYPH_52 */
    { ATLAS_FONT, 268, 45, 36, 40 }, /* GLYPH_53 */
    { ATLAS_FONT, 307, 45, 34, 40 }, /* GLYPH_54 */
    { ATLAS_FONT, 344, 45, 43, 40 }, /* GLYPH_55 */
    { ATLAS_FONT, 390, 45, 42, 40 }, /* GLYPH_56 */
    { ATLAS_FONT, 435, 45, 50, 40 }, /* GLYPH_57 */
    { ATLAS_FONT, 2, 88, 46, 40 }, /* GLYPH_58 */
    { ATLAS_FONT, 51, 88, 42, 40 }, /* GLYPH_59 */
    { ATLAS_FONT, 96, 88, 41, 40 }, /* GLYPH_5A */
    { ATLAS_FONT, 140, 88, 39, 40 }, /* GLYPH_30 */
    { ATLAS_FONT, 182, 88, 28, 40 }, /* GLYPH_31 */
    { ATLAS_FONT, 213, 88, 38, 40 }, /* GLYPH_32 */
    { ATLAS_FONT, 254, 88, 37, 40 }, /* GLYPH_33 */
    { ATLAS_FONT, 294, 88, 38, 40 }, /* GLYPH_34 */
    { ATLAS_FONT, 335, 88, 38, 40 }, /* GLYPH_35 */
    { ATLAS_FONT, 376, 88, 36, 40 }, /* GLYPH_36 */
    { ATLAS_FONT, 415, 88, 36, 40 }, /* GLYPH_37 */
    { ATLAS_FONT, 454, 88, 36, 40 }, /* GLYPH_38 */
    { ATLAS_FONT, 2, 131, 34, 40 }, /* GLYPH_39 */
    { ATLAS_FONT, 39, 131, 19, 40 }, /* GLYPH_21 */
    { ATLAS_FONT, 61, 131, 25, 40 }, /* GLYPH_3F */
    { ATLAS_FONT, 89, 131, 12, 40 }, /* GLYPH_2E */
    { ATLAS_FONT, 104, 131, 13, 40 }, /* GLYPH_2C */
    { ATLAS_FONT, 120, 131, 13, 40 }, /* GLYPH_3A */
    { ATLAS_FONT, 136, 131, 15, 40 }, /* GLYPH_3B */
    { ATLAS_FONT, 154, 131, 12, 40 }, /* GLYPH_27 */
    { ATLAS_FONT, 169, 131, 17, 40 }, /* GLYPH_22 */
    { ATLAS_FONT, 189, 131, 18, 40 }, /* GLYPH_28 */
    { ATLAS_FONT, 210, 131, 18, 40 }, /* GLYPH_29 */
    { ATLAS_FONT, 231, 131, 19, 40 }, /* GLYPH_5B */
    { ATLAS_FONT, 253, 131, 18, 40 }, /* GLYPH_5D */
    { ATLAS_FONT, 274, 131, 23, 40 }, /* GLYPH_2B */
    { ATLAS_FONT, 300, 131, 21, 40 }, /* GLYPH_2D */
    { ATLAS_FONT, 324, 131, 22, 40 }, /* GLYPH_3D */
    { ATLAS_FONT, 349, 131, 31, 40 }, /* GLYPH_25 */
    { ATLAS_FONT, 383, 131, 50, 40 }, /* GLYPHW_41 */
    { ATLAS_FONT, 436, 131, 38, 40 }, /* GLYPHW_42 */
    { ATLAS_FONT, 2, 174, 40, 40 }, /* GLYPHW_43 */
    { ATLAS_FONT, 45, 174, 41, 40 }, /* GLYPHW_44 */
    { ATLAS_FONT, 89, 174, 39, 40 }, /* GLYPHW_45 */
    { ATLAS_FONT, 131, 174, 39, 40 }, /* GLYPHW_46 */
    { ATLAS_FONT, 173, 174, 38, 40 }, /* GLYPHW_47 */
    { ATLAS_FONT, 214, 174, 42, 40 }, /* GLYPHW_48 */
    { ATLAS_FONT, 259, 174, 26, 40 }, /* GLYPHW_49 */
    { ATLAS_FONT, 288, 174, 36, 40 }, /* GLYPHW_4A */
    { ATLAS_FONT, 327, 174, 46, 40 }, /* GLYPHW_4B */
    { ATLAS_FONT, 376, 174, 36, 40 }, /* GLYPHW_4C */
    { ATLAS_FONT, 415, 174, 50, 40 }, /* GLYPHW_4D */
    { ATLAS_FONT, 2, 217, 44, 40 }, /* GLYPHW_4E */
    { ATLAS_FONT, 49, 217, 39, 40 }, /* GLYPHW_4F */
    { ATLAS_FONT, 91, 217, 38, 40 }, /* GLYPHW_50 */
    { ATLAS_FONT, 132, 217, 39, 40 }, /* GLYPHW_51 */
    { ATLAS_FONT, 174, 217, 38, 40 }, /* GLYPHW_52 */
    { ATLAS_FONT, 215, 217, 36, 40 }, /* GLYPHW_53 */
    { ATLAS_FONT, 254, 217, 34, 40 }, /* GLYPHW_54 */
    { ATLAS_FONT, 291, 217, 43, 40 }, /* GLYPHW_55 */
    { ATLAS_FONT, 337, 217, 42, 40 }, /* GLYPHW_56 */
    { ATLAS_FONT, 382, 217, 50, 40 }, /* GLYPHW_57 */
    { ATLAS_FONT, 435, 217, 46, 40 }, /* GLYPHW_58 */
    { ATLAS_FONT, 2, 260, 42, 40 }, /* GLYPHW_59 */
    { ATLAS_FONT, 47, 260, 41, 40 }, /* GLYPHW_5A */
    { ATLAS_FONT, 91, 260, 39, 40 }, /* GLYPHW_30 */
    { ATLAS_FONT, 133, 260, 28, 40 }, /* GLYPHW_31 */
    { ATLAS_FONT, 164, 260, 38, 40 }, /* GLYPHW_32 */
    { ATLAS_FONT, 205, 260, 37, 40 }, /* GLYPHW_33 */
    { ATLAS_FONT, 245, 260, 38, 40 }, /* GLYPHW_34 */
    { ATLAS_FONT, 286, 260, 38, 40 }, /* GLYPHW_35 */
    { ATLAS_FONT, 327, 260, 36, 40 }, /* GLYPHW_36 */
    { ATLAS_FONT, 366, 260, 36, 40 }, /* GLYPHW_37 */
    { ATLAS_FONT, 405, 260, 36, 40 }, /* GLYPHW_38 */
    { ATLAS_FONT, 444, 260, 34, 40 }, /* GLYPHW_39 */
    { ATLAS_FONT, 481, 260, 19, 40 }, /* GLYPHW_21 */
    { ATLAS_FONT, 2, 303, 25, 40 }, /* GLYPHW_3F */
    { ATLAS_FONT, 30, 303, 12, 40 }, /* GLYPHW_2E */
    { ATLAS_FONT, 45, 303, 13, 40 }, /* GLYPHW_2C */
    { ATLAS_FONT, 61, 303, 13, 40 }, /* GLYPHW_3A */
    { ATLAS_FONT, 77, 303, 15, 40 }, /* GLYPHW_3B */
    { ATLAS_FONT, 95, 303, 12, 40 }, /* GLYPHW_27 */
    { ATLAS_FONT, 110, 303, 17, 40 }, /* GLYPHW_22 */
    { ATLAS_FONT, 130, 303, 18, 40 }, /* GLYPHW_28 */
    { ATLAS_FONT, 151, 303, 18, 40 }, /* GLYPHW_29 */
    { ATLAS_FONT, 172, 303, 19, 40 }, /* GLYPHW_5B */
    { ATLAS_FONT, 194, 303, 18, 40 }, /* GLYPHW_5D */
    { ATLAS_FONT, 215, 303, 23, 40 }, /* GLYPHW_2B */
    { ATLAS_FONT, 241, 303, 21, 40 }, /* GLYPHW_2D */
    { ATLAS_FONT, 265, 303, 22, 40 }, /* GLYPHW_3D */
    { ATLAS_FONT, 290, 303, 31, 40 }, /* GLYPHW_25 */
};

const short glyph_sprite[128] = {
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    SPR_GLYPH_21,
    SPR_GLYPH_22,
    -1,
    -1,
    SPR_GLYPH_25,
    -1,
    SPR_GLYPH_27,
    SPR_GLYPH_28,
    SPR_GLYPH_29,
    -1,
    SPR_GLYPH_2B,
    SPR_GLYPH_2C,
    SPR_GLYPH_2D,
    SPR_GLYPH_2E,
    -1,
    SPR_GLYPH_30,
    SPR_GLYPH_31,
    SPR_GLYPH_32,
    SPR_GLYPH_33,
    SPR_GLYPH_34,
    SPR_GLYPH_35,
    SPR_GLYPH_36,
    SPR_GLYPH_37,
    SPR_GLYPH_38,
    SPR_GLYPH_39,
    SPR_GLYPH_3A,
    SPR_GLYPH_3B,
    -1,
    SPR_GLYPH_3D,
    -1,
    SPR_GLYPH_3F,
    -1,
    SPR_GLYPH_41,
    SPR_GLYPH_42,
    SPR_GLYPH_43,
    SPR_GLYPH_44,
    SPR_GLYPH_45,
    SPR_GLYPH_46,
    SPR_GLYPH_47,
    SPR_GLYPH_48,
    SPR_GLYPH_49,
    SPR_GLYPH_4A,
    SPR_GLYPH_4B,
    SPR_GLYPH_4C,
    SPR_GLYPH_4D,
    SPR_GLYPH_4E,
    SPR_GLYPH_4F,
    SPR_GLYPH_50,
    SPR_GLYPH_51,
    SPR_GLYPH_52,
    SPR_GLYPH_53,
    SPR_GLYPH_54,
    SPR_GLYPH_55,
    SPR_GLYPH_56,
    SPR_GLYPH_57,
    SPR_GLYPH_58,
    SPR_GLYPH_59,
    SPR_GLYPH_5A,
    SPR_GLYPH_5B,
    -1,
    SPR_GLYPH_5D,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
};

const short glyph_sprite_plain[128] = {
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    SPR_GLYPHW_21,
    SPR_GLYPHW_22,
    -1,
    -1,
    SPR_GLYPHW_25,
    -1,
    SPR_GLYPHW_27,
    SPR_GLYPHW_28,
    SPR_GLYPHW_29,
    -1,
    SPR_GLYPHW_2B,
    SPR_GLYPHW_2C,
    SPR_GLYPHW_2D,
    SPR_GLYPHW_2E,
    -1,
    SPR_GLYPHW_30,
    SPR_GLYPHW_31,
    SPR_GLYPHW_32,
    SPR_GLYPHW_33,
    SPR_GLYPHW_34,
    SPR_GLYPHW_35,
    SPR_GLYPHW_36,
    SPR_GLYPHW_37,
    SPR_GLYPHW_38,
    SPR_GLYPHW_39,
    SPR_GLYPHW_3A,
    SPR_GLYPHW_3B,
    -1,
    SPR_GLYPHW_3D,
    -1,
    SPR_GLYPHW_3F,
    -1,
    SPR_GLYPHW_41,
    SPR_GLYPHW_42,
    SPR_GLYPHW_43,
    SPR_GLYPHW_44,
    SPR_GLYPHW_45,
    SPR_GLYPHW_46,
    SPR_GLYPHW_47,
    SPR_GLYPHW_48,
    SPR_GLYPHW_49,
    SPR_GLYPHW_4A,
    SPR_GLYPHW_4B,
    SPR_GLYPHW_4C,
    SPR_GLYPHW_4D,
    SPR_GLYPHW_4E,
    SPR_GLYPHW_4F,
    SPR_GLYPHW_50,
    SPR_GLYPHW_51,
    SPR_GLYPHW_52,
    SPR_GLYPHW_53,
    SPR_GLYPHW_54,
    SPR_GLYPHW_55,
    SPR_GLYPHW_56,
    SPR_GLYPHW_57,
    SPR_GLYPHW_58,
    SPR_GLYPHW_59,
    SPR_GLYPHW_5A,
    SPR_GLYPHW_5B,
    -1,
    SPR_GLYPHW_5D,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
};

// Formatting library for C++ - the core API
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.
//
// Copyright (c) 2018 - present, Remotion (Igor Schulz)
// All Rights Reserved
// {fmt} support for rgb color output.

#ifndef FMT_COLORS_H_
#define FMT_COLORS_H_


#include "format.h"

FMT_BEGIN_NAMESPACE

// rgb is a struct for red, green and blue colors.
// we use rgb as name because some editors will show it as color direct in the editor.
struct rgb {
  FMT_CONSTEXPR_DECL rgb() : r(0), g(0), b(0) {}
  FMT_CONSTEXPR_DECL rgb(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
  FMT_CONSTEXPR_DECL rgb(uint32_t hex) 
      : r((hex >> 16) & 0xFF), g((hex >> 8) & 0xFF), b((hex) & 0xFF) {}
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

namespace internal {

FMT_CONSTEXPR inline void to_esc(uint8_t c, char out[], int offset) {
  out[offset + 0] = static_cast<char>('0' + c / 100);
  out[offset + 1] = static_cast<char>('0' + c / 10 % 10);
  out[offset + 2] = static_cast<char>('0' + c % 10);
}

} // namespace internal

FMT_FUNC void vprint_rgb(rgb fd, string_view format, format_args args) {
  char escape_fd[] = "\x1b[38;2;000;000;000m";
  static FMT_CONSTEXPR_DECL const char RESET_COLOR[] = "\x1b[0m";
  internal::to_esc(fd.r, escape_fd, 7);
  internal::to_esc(fd.g, escape_fd, 11);
  internal::to_esc(fd.b, escape_fd, 15);
  
  std::fputs(escape_fd, stdout);
  vprint(format, args);
  std::fputs(RESET_COLOR, stdout);
}


FMT_FUNC void vprint_rgb(rgb fd, rgb bg, string_view format, format_args args) {
  char escape_fd[] = "\x1b[38;2;000;000;000m"; // foreground color
  char escape_bg[] = "\x1b[48;2;000;000;000m"; // background color
  static FMT_CONSTEXPR_DECL const char RESET_COLOR[] = "\x1b[0m";
  internal::to_esc(fd.r, escape_fd, 7);
  internal::to_esc(fd.g, escape_fd, 11);
  internal::to_esc(fd.b, escape_fd, 15);
  
  internal::to_esc(bg.r, escape_bg, 7);
  internal::to_esc(bg.g, escape_bg, 11);
  internal::to_esc(bg.b, escape_bg, 15);
  
  std::fputs(escape_fd, stdout);
  std::fputs(escape_bg, stdout);
  vprint(format, args);
  std::fputs(RESET_COLOR, stdout);
}

template <typename... Args>
inline void print_rgb(rgb fd, string_view format_str, const Args & ... args) {
  vprint_rgb(fd, format_str, make_format_args(args...));
}

// rgb foreground color
template <typename... Args>
inline void print(rgb fd, string_view format_str, const Args & ... args) {
  vprint_rgb(fd, format_str, make_format_args(args...));
}

// rgb foreground color and background color
template <typename... Args>
inline void print(rgb fd, rgb bg, string_view format_str, const Args & ... args) {
  vprint_rgb(fd, bg, format_str, make_format_args(args...));
}


enum hex : uint32_t { // use enum class ?
  AliceBlue            = 0xF0F8FF, // rgb(240,248,255); 
  AntiqueWhite         = 0xFAEBD7, // rgb(250,235,215); 
  Aqua                 = 0x00FFFF, // rgb(0,255,255);   
  Aquamarine           = 0x7FFFD4, // rgb(127,255,212); 
  Azure                = 0xF0FFFF, // rgb(240,255,255); 
  Beige                = 0xF5F5DC, // rgb(245,245,220); 
  Bisque               = 0xFFE4C4, // rgb(255,228,196); 
  Black                = 0x000000, // rgb(0,0,0);       
  BlanchedAlmond       = 0xFFEBCD, // rgb(255,235,205); 
  Blue                 = 0x0000FF, // rgb(0,0,255);     
  BlueViolet           = 0x8A2BE2, // rgb(138,43,226);  
  Brown                = 0xA52A2A, // rgb(165,42,42);   
  BurlyWood            = 0xDEB887, // rgb(222,184,135); 
  CadetBlue            = 0x5F9EA0, // rgb(95,158,160);  
  Chartreuse           = 0x7FFF00, // rgb(127,255,0);   
  Chocolate            = 0xD2691E, // rgb(210,105,30);  
  Coral                = 0xFF7F50, // rgb(255,127,80);  
  CornflowerBlue       = 0x6495ED, // rgb(100,149,237); 
  Cornsilk             = 0xFFF8DC, // rgb(255,248,220); 
  Crimson              = 0xDC143C, // rgb(220,20,60);   
  Cyan                 = 0x00FFFF, // rgb(0,255,255);   
  DarkBlue             = 0x00008B, // rgb(0,0,139);     
  DarkCyan             = 0x008B8B, // rgb(0,139,139);   
  DarkGoldenRod        = 0xB8860B, // rgb(184,134,11);  
  DarkGray             = 0xA9A9A9, // rgb(169,169,169); 
  DarkGreen            = 0x006400, // rgb(0,100,0);     
  DarkKhaki            = 0xBDB76B, // rgb(189,183,107); 
  DarkMagenta          = 0x8B008B, // rgb(139,0,139);   
  DarkOliveGreen       = 0x556B2F, // rgb(85,107,47);   
  DarkOrange           = 0xFF8C00, // rgb(255,140,0);   
  DarkOrchid           = 0x9932CC, // rgb(153,50,204);  
  DarkRed              = 0x8B0000, // rgb(139,0,0);     
  DarkSalmon           = 0xE9967A, // rgb(233,150,122); 
  DarkSeaGreen         = 0x8FBC8F, // rgb(143,188,143); 
  DarkSlateBlue        = 0x483D8B, // rgb(72,61,139);   
  DarkSlateGray        = 0x2F4F4F, // rgb(47,79,79);    
  DarkTurquoise        = 0x00CED1, // rgb(0,206,209);   
  DarkViolet           = 0x9400D3, // rgb(148,0,211);   
  DeepPink             = 0xFF1493, // rgb(255,20,147);  
  DeepSkyBlue          = 0x00BFFF, // rgb(0,191,255);   
  DimGray              = 0x696969, // rgb(105,105,105); 
  DodgerBlue           = 0x1E90FF, // rgb(30,144,255);  
  FireBrick            = 0xB22222, // rgb(178,34,34);   
  FloralWhite          = 0xFFFAF0, // rgb(255,250,240); 
  ForestGreen          = 0x228B22, // rgb(34,139,34);   
  Fuchsia              = 0xFF00FF, // rgb(255,0,255);   
  Gainsboro            = 0xDCDCDC, // rgb(220,220,220); 
  GhostWhite           = 0xF8F8FF, // rgb(248,248,255); 
  Gold                 = 0xFFD700, // rgb(255,215,0);   
  GoldenRod            = 0xDAA520, // rgb(218,165,32);  
  Gray                 = 0x808080, // rgb(128,128,128); 
  Green                = 0x008000, // rgb(0,128,0);     
  GreenYellow          = 0xADFF2F, // rgb(173,255,47);  
  HoneyDew             = 0xF0FFF0, // rgb(240,255,240); 
  HotPink              = 0xFF69B4, // rgb(255,105,180); 
  IndianRed            = 0xCD5C5C, // rgb(205,92,92);   
  Indigo               = 0x4B0082, // rgb(75,0,130);    
  Ivory                = 0xFFFFF0, // rgb(255,255,240); 
  Khaki                = 0xF0E68C, // rgb(240,230,140); 
  Lavender             = 0xE6E6FA, // rgb(230,230,250); 
  LavenderBlush        = 0xFFF0F5, // rgb(255,240,245); 
  LawnGreen            = 0x7CFC00, // rgb(124,252,0);   
  LemonChiffon         = 0xFFFACD, // rgb(255,250,205); 
  LightBlue            = 0xADD8E6, // rgb(173,216,230); 
  LightCoral           = 0xF08080, // rgb(240,128,128); 
  LightCyan            = 0xE0FFFF, // rgb(224,255,255); 
  LightGoldenRodYellow = 0xFAFAD2, // rgb(250,250,210); 
  LightGray            = 0xD3D3D3, // rgb(211,211,211); 
  LightGreen           = 0x90EE90, // rgb(144,238,144); 
  LightPink            = 0xFFB6C1, // rgb(255,182,193); 
  LightSalmon          = 0xFFA07A, // rgb(255,160,122); 
  LightSeaGreen        = 0x20B2AA, // rgb(32,178,170);  
  LightSkyBlue         = 0x87CEFA, // rgb(135,206,250); 
  LightSlateGray       = 0x778899, // rgb(119,136,153); 
  LightSteelBlue       = 0xB0C4DE, // rgb(176,196,222); 
  LightYellow          = 0xFFFFE0, // rgb(255,255,224); 
  Lime                 = 0x00FF00, // rgb(0,255,0);     
  LimeGreen            = 0x32CD32, // rgb(50,205,50);   
  Linen                = 0xFAF0E6, // rgb(250,240,230); 
  Magenta              = 0xFF00FF, // rgb(255,0,255);   
  Maroon               = 0x800000, // rgb(128,0,0);     
  MediumAquaMarine     = 0x66CDAA, // rgb(102,205,170); 
  MediumBlue           = 0x0000CD, // rgb(0,0,205);     
  MediumOrchid         = 0xBA55D3, // rgb(186,85,211);  
  MediumPurple         = 0x9370DB, // rgb(147,112,219); 
  MediumSeaGreen       = 0x3CB371, // rgb(60,179,113);  
  MediumSlateBlue      = 0x7B68EE, // rgb(123,104,238); 
  MediumSpringGreen    = 0x00FA9A, // rgb(0,250,154);   
  MediumTurquoise      = 0x48D1CC, // rgb(72,209,204);  
  MediumVioletRed      = 0xC71585, // rgb(199,21,133);  
  MidnightBlue         = 0x191970, // rgb(25,25,112);   
  MintCream            = 0xF5FFFA, // rgb(245,255,250); 
  MistyRose            = 0xFFE4E1, // rgb(255,228,225); 
  Moccasin             = 0xFFE4B5, // rgb(255,228,181); 
  NavajoWhite          = 0xFFDEAD, // rgb(255,222,173); 
  Navy                 = 0x000080, // rgb(0,0,128);     
  OldLace              = 0xFDF5E6, // rgb(253,245,230); 
  Olive                = 0x808000, // rgb(128,128,0);   
  OliveDrab            = 0x6B8E23, // rgb(107,142,35);  
  Orange               = 0xFFA500, // rgb(255,165,0);   
  OrangeRed            = 0xFF4500, // rgb(255,69,0);    
  Orchid               = 0xDA70D6, // rgb(218,112,214); 
  PaleGoldenRod        = 0xEEE8AA, // rgb(238,232,170); 
  PaleGreen            = 0x98FB98, // rgb(152,251,152); 
  PaleTurquoise        = 0xAFEEEE, // rgb(175,238,238); 
  PaleVioletRed        = 0xDB7093, // rgb(219,112,147); 
  PapayaWhip           = 0xFFEFD5, // rgb(255,239,213); 
  PeachPuff            = 0xFFDAB9, // rgb(255,218,185); 
  Peru                 = 0xCD853F, // rgb(205,133,63);  
  Pink                 = 0xFFC0CB, // rgb(255,192,203); 
  Plum                 = 0xDDA0DD, // rgb(221,160,221); 
  PowderBlue           = 0xB0E0E6, // rgb(176,224,230); 
  Purple               = 0x800080, // rgb(128,0,128);   
  RebeccaPurple        = 0x663399, // rgb(102,51,153);  
  Red                  = 0xFF0000, // rgb(255,0,0);     
  RosyBrown            = 0xBC8F8F, // rgb(188,143,143); 
  RoyalBlue            = 0x4169E1, // rgb(65,105,225);  
  SaddleBrown          = 0x8B4513, // rgb(139,69,19);   
  Salmon               = 0xFA8072, // rgb(250,128,114); 
  SandyBrown           = 0xF4A460, // rgb(244,164,96);  
  SeaGreen             = 0x2E8B57, // rgb(46,139,87);   
  SeaShell             = 0xFFF5EE, // rgb(255,245,238); 
  Sienna               = 0xA0522D, // rgb(160,82,45);   
  Silver               = 0xC0C0C0, // rgb(192,192,192); 
  SkyBlue              = 0x87CEEB, // rgb(135,206,235); 
  SlateBlue            = 0x6A5ACD, // rgb(106,90,205);  
  SlateGray            = 0x708090, // rgb(112,128,144); 
  Snow                 = 0xFFFAFA, // rgb(255,250,250); 
  SpringGreen          = 0x00FF7F, // rgb(0,255,127);   
  SteelBlue            = 0x4682B4, // rgb(70,130,180);  
  Tan                  = 0xD2B48C, // rgb(210,180,140); 
  Teal                 = 0x008080, // rgb(0,128,128);   
  Thistle              = 0xD8BFD8, // rgb(216,191,216); 
  Tomato               = 0xFF6347, // rgb(255,99,71);   
  Turquoise            = 0x40E0D0, // rgb(64,224,208);  
  Violet               = 0xEE82EE, // rgb(238,130,238); 
  Wheat                = 0xF5DEB3, // rgb(245,222,179); 
  White                = 0xFFFFFF, // rgb(255,255,255); 
  WhiteSmoke           = 0xF5F5F5, // rgb(245,245,245); 
  Yellow               = 0xFFFF00, // rgb(255,255,0);   
  YellowGreen          = 0x9ACD32, // rgb(154,205,50);  
}; // enum hex

FMT_END_NAMESPACE


#endif // FMT_COLORS_H_

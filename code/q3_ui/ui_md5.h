/*
===========================================================================
ui_md5.h — MD5 for UI (status signature check)
===========================================================================
*/

#ifndef UI_MD5_INCLUDE_H
#define UI_MD5_INCLUDE_H

void UI_MD5_Buffer(const unsigned char* data, int len, unsigned char digest[16]);
void UI_MD5_HexLower(const unsigned char digest[16], char* hexOut);

#endif /* UI_MD5_INCLUDE_H */

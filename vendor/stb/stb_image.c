#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
static unsigned char* load_bmp(const char* filename, int* x, int* y, int* channels_out) {
FILE* f= fopen(filename, "rb");
if(!f) return NULL;
unsigned char header[54];
if(fread(header, 1, 54, f) != 54) {
fclose(f);
return NULL;
}
if(header[0] != 'B' || header[1] != 'M') {
fclose(f);
return NULL;
}
unsigned int dataOffset= *(unsigned int*)&header[10];
unsigned int width= *(unsigned int*)&header[18];
unsigned int height= *(unsigned int*)&header[22];
unsigned short bpp= *(unsigned short*)&header[28];
if(bpp != 24 && bpp != 32) {
fclose(f);
return NULL;
}
/* Sanity checks to avoid excessive allocations and integer overflow */
const unsigned int MAX_DIM= 65536U;
if(width == 0 || height == 0 || width > MAX_DIM || height > MAX_DIM) {
fclose(f);
return NULL;
}
/* compute rowSize and ensure it doesn't overflow */
unsigned long rowSize_ul= ((unsigned long)bpp * (unsigned long)width + 31UL) / 32UL * 4UL;
if(rowSize_ul > SIZE_MAX) {
fclose(f);
return NULL;
}
size_t rowSize= (size_t)rowSize_ul;
/* check raw data size */
if((unsigned long)height > SIZE_MAX / rowSize) {
fclose(f);
return NULL;
}
size_t dataSize= (size_t)rowSize * (size_t)height;
const size_t MAX_RAW_BYTES= (size_t)256 * 1024 * 1024; /* 256MB */
if(dataSize == 0 || dataSize > MAX_RAW_BYTES) {
fclose(f);
return NULL;
}
/* output buffer size check (do multiplications safely to avoid overflow) */
if((size_t)width > SIZE_MAX / (size_t)height) {
fclose(f);
return NULL;
}
size_t pixels= (size_t)width * (size_t)height;
if(pixels > SIZE_MAX / 4) {
fclose(f);
return NULL;
}
size_t out_size= pixels * 4;
const size_t MAX_OUT_BYTES= (size_t)512 * 1024 * 1024; /* 512MB */
if(out_size == 0 || out_size > MAX_OUT_BYTES) {
fclose(f);
return NULL;
}
unsigned char* data= (unsigned char*)malloc(out_size);
if(!data) {
fclose(f);
return NULL;
}
fseek(f, dataOffset, SEEK_SET);
unsigned char* raw= (unsigned char*)malloc(dataSize);
if(fread(raw, 1, dataSize, f) != dataSize) {
free(raw);
free(data);
fclose(f);
return NULL;
}
fclose(f);
for(unsigned int row= 0; row < height; row++) {
unsigned char* src= raw + (height - 1 - row) * rowSize;
for(unsigned int col= 0; col < width; col++) {
unsigned char b= src[col * (bpp / 8) + 0];
unsigned char g= src[col * (bpp / 8) + 1];
unsigned char r= src[col * (bpp / 8) + 2];
unsigned char a= (bpp == 32) ? src[col * 4 + 3] : 255;
size_t idx= (size_t)row * (size_t)width + col;
data[idx * 4 + 0]= r;
data[idx * 4 + 1]= g;
data[idx * 4 + 2]= b;
data[idx * 4 + 3]= a;
}
}
free(raw);
*x= (int)width;
*y= (int)height;
*channels_out= 4;
return data;
}
static unsigned int read_be_u32(const unsigned char* p) { return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) | ((unsigned int)p[2] << 8) | (unsigned int)p[3]; }
static unsigned char* load_png(const char* filename, int* x, int* y, int* channels_out) {
FILE* f= fopen(filename, "rb");
if(!f) return NULL;
unsigned char sig[8];
if(fread(sig, 1, 8, f) != 8) {
fclose(f);
return NULL;
}
const unsigned char png_sig[8]= {137, 'P', 'N', 'G', 13, 10, 26, 10};
if(memcmp(sig, png_sig, 8) != 0) {
fclose(f);
return NULL;
}
unsigned char* idat= NULL;
size_t idat_len= 0;
unsigned int width= 0, height= 0;
int bit_depth= 0, color_type= 0, compression= 0, filter= 0, interlace= 0;
int seen_IHDR= 0;
unsigned char* plte= NULL;
size_t plte_len= 0;
unsigned char* trns= NULL;
size_t trns_len= 0;
while(1) {
unsigned char lenb[4];
if(fread(lenb, 1, 4, f) != 4) break;
unsigned int len= read_be_u32(lenb);
/* Cap per-chunk length to protect against fuzzed huge chunks */
const unsigned int MAX_PNG_CHUNK= 64U * 1024U * 1024U; /* 64MB */
if(len > MAX_PNG_CHUNK) {
free(idat);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
fclose(f);
return NULL;
}
char type[5]= {0};
if(fread(type, 1, 4, f) != 4) break;
unsigned char* data= NULL;
if(len > 0) {
data= (unsigned char*)malloc(len);
if(!data) {
free(idat);
fclose(f);
return NULL;
}
if(fread(data, 1, len, f) != len) {
free(data);
free(idat);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
fclose(f);
return NULL;
}
}
unsigned char crc[4];
if(fread(crc, 1, 4, f) != 4) {
free(data);
free(idat);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
fclose(f);
return NULL;
}
if(strcmp(type, "IHDR") == 0) {
if(len < 13) {
free(data);
free(idat);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
fclose(f);
return NULL;
}
width= read_be_u32(data);
height= read_be_u32(data + 4);
bit_depth= data[8];
color_type= data[9];
compression= data[10];
filter= data[11];
interlace= data[12];
seen_IHDR= 1;
} else if(strcmp(type, "PLTE") == 0) {
if(len == 0 || len % 3 != 0) {
free(data);
free(idat);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
fclose(f);
return NULL;
}
if(plte) {
free(plte);
plte= NULL;
}
plte= (unsigned char*)malloc(len);
if(!plte) {
free(data);
free(idat);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
fclose(f);
return NULL;
}
/* PLTE chunk seen */
memcpy(plte, data, len);
plte_len= len;
} else if(strcmp(type, "tRNS") == 0) {
/* tRNS must have a positive length; reject empty tRNS to avoid NULL memcpy/UB and unexpected states */
if(len == 0) {
free(data);
free(idat);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
fclose(f);
return NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
trns= (unsigned char*)malloc(len);
if(!trns) {
free(data);
free(idat);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
fclose(f);
return NULL;
}
/* tRNS chunk seen */
if(data && len > 0) memcpy(trns, data, len);
trns_len= len;
} else if(strcmp(type, "IDAT") == 0) {
/* Ignore zero-length IDAT chunks to avoid calling realloc(..., 0) which may free the
                                                                           buffer and lead to double-free when we
               subsequently free `idat`. */
if(len > 0) {
unsigned char* nbuf= (unsigned char*)realloc(idat, idat_len + len);
if(!nbuf) {
free(data);
free(idat);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
fclose(f);
return NULL;
}
idat= nbuf;
if(data) memcpy(idat + idat_len, data, len);
idat_len+= len;
}
} else if(strcmp(type, "IEND") == 0) {
free(data);
break;
}
free(data);
}
if(!seen_IHDR || idat_len == 0) {
free(idat);
if(plte) free(plte);
if(trns) free(trns);
fclose(f);
return NULL;
}
if(bit_depth != 8 || compression != 0 || filter != 0 || interlace != 0) {
free(idat);
if(plte) free(plte);
if(trns) free(trns);
fclose(f);
return NULL;
}
size_t channels;
if(color_type == 6)
channels= 4;
else if(color_type == 2)
channels= 3;
else if(color_type == 3)
channels= 1;
else
channels= 0;
if(channels == 0) {
free(idat);
if(plte) free(plte);
if(trns) free(trns);
fclose(f);
return NULL;
}
/* Sanity-check dimensions to avoid excessive allocations and integer overflow */
if(width == 0 || height == 0) {
free(idat);
if(plte) free(plte);
if(trns) free(trns);
fclose(f);
return NULL;
}
/* Reject ridiculously large dimensions (protect against fuzzed IHDR with huge values) */
const unsigned int MAX_DIM= 65536U;
if(width > MAX_DIM || height > MAX_DIM) {
free(idat);
if(plte) free(plte);
if(trns) free(trns);
fclose(f);
return NULL;
}
/* Check for multiplication overflow when computing expected decompressed size */
size_t channels_s= channels;
if(channels_s > 0) {
if(width > SIZE_MAX / channels_s) {
free(idat);
if(plte) free(plte);
if(trns) free(trns);
fclose(f);
return NULL;
}
size_t row_bytes= (size_t)width * channels_s;
/* (1 + row_bytes) * height must not overflow */
if(row_bytes > SIZE_MAX - 1 || height > SIZE_MAX / (row_bytes + 1)) {
free(idat);
if(plte) free(plte);
if(trns) free(trns);
fclose(f);
return NULL;
}
}
size_t expected= (size_t)(1 + (size_t)width * channels) * (size_t)height;
/* Protect against unreasonably large decompressed images from fuzzed IHDR values. */
const size_t MAX_DECOMPRESSED_BYTES= (size_t)256 * 1024 * 1024; /* 256MB */
if(expected == 0 || expected > MAX_DECOMPRESSED_BYTES) {
free(idat);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
fclose(f);
return NULL;
}
unsigned char* decompressed= (unsigned char*)malloc(expected);
if(!decompressed) {
free(idat);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
fclose(f);
return NULL;
}
uLongf destLen= (uLongf)expected;
int zret= uncompress(decompressed, &destLen, idat, (uLong)idat_len);
free(idat);
fclose(f);
if(zret != Z_OK || destLen != expected) {
free(decompressed);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
return NULL;
}
size_t out_size= (size_t)width * (size_t)height * 4;
const size_t MAX_OUT_BYTES= (size_t)512 * 1024 * 1024; /* 512MB */
if(out_size == 0 || out_size > MAX_OUT_BYTES) {
free(decompressed);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
return NULL;
}
unsigned char* out= (unsigned char*)malloc(out_size);
if(!out) {
free(decompressed);
return NULL;
}
unsigned char* prev_row= NULL;
unsigned char* recon= (unsigned char*)malloc((size_t)width * channels);
if(!recon) {
free(decompressed);
free(out);
return NULL;
}
for(size_t yrow= 0; yrow < (size_t)height; yrow++) {
unsigned char* scan= decompressed + yrow * ((size_t)width * channels + 1);
unsigned char filter_type= scan[0];
unsigned char* raw= scan + 1;
for(size_t i= 0; i < (size_t)width * channels; i++) {
unsigned char left= (i >= channels) ? recon[i - channels] : 0;
unsigned char up= prev_row ? prev_row[i] : 0;
unsigned char up_left= (prev_row && i >= channels) ? prev_row[i - channels] : 0;
unsigned char val= raw[i];
unsigned char res= 0;
switch(filter_type) {
case 0: res= val; break;
case 1: res= (unsigned char)(val + left); break;
case 2: res= (unsigned char)(val + up); break;
case 3: {
unsigned int avg= ((unsigned int)left + (unsigned int)up) >> 1;
res= (unsigned char)(val + (unsigned char)avg);
} break;
case 4: {
int p= (int)left + (int)up - (int)up_left;
int pa= abs(p - (int)left);
int pb= abs(p - (int)up);
int pc= abs(p - (int)up_left);
int pr= (pa <= pb && pa <= pc) ? (int)left : (pb <= pc ? (int)up : (int)up_left);
res= (unsigned char)(val + (unsigned char)pr);
} break;
default:
free(recon);
free(decompressed);
free(out);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
return NULL;
}
recon[i]= res;
}
for(size_t xpix= 0; xpix < (size_t)width; xpix++) {
size_t in_idx= xpix * channels;
size_t out_idx= (yrow * (size_t)width + xpix) * 4;
if(color_type == 3) {
unsigned char idx= recon[in_idx];
if((size_t)idx * 3 + 2 >= plte_len) {
free(recon);
free(decompressed);
free(out);
if(plte) free(plte);
if(trns) free(trns);
return NULL;
}
unsigned char r= plte[(size_t)idx * 3 + 0];
unsigned char g= plte[(size_t)idx * 3 + 1];
unsigned char b= plte[(size_t)idx * 3 + 2];
unsigned char a= (trns && (size_t)idx < trns_len) ? trns[(size_t)idx] : 255;
out[out_idx + 0]= r;
out[out_idx + 1]= g;
out[out_idx + 2]= b;
out[out_idx + 3]= a;
} else {
unsigned char r= recon[in_idx + 0];
unsigned char g= recon[in_idx + 1];
unsigned char b= recon[in_idx + 2];
unsigned char a= (channels == 4) ? recon[in_idx + 3] : 255;
out[out_idx + 0]= r;
out[out_idx + 1]= g;
out[out_idx + 2]= b;
out[out_idx + 3]= a;
}
}
if(prev_row) free(prev_row);
prev_row= (unsigned char*)malloc((size_t)width * channels);
if(!prev_row) {
free(recon);
free(decompressed);
free(out);
if(plte) {
free(plte);
plte= NULL;
}
if(trns) {
free(trns);
trns= NULL;
}
return NULL;
}
memcpy(prev_row, recon, (size_t)width * channels);
}
free(recon);
free(decompressed);
if(prev_row) free(prev_row);
*x= (int)width;
*y= (int)height;
*channels_out= 4;
if(plte) free(plte);
if(trns) free(trns);
return out;
}
unsigned char* stbi_load(char const* filename, int* x, int* y, int* channels_in_file, int desired_channels) {
(void)desired_channels;
unsigned char sig[8];
FILE* f= fopen(filename, "rb");
if(!f) return NULL;
if(fread(sig, 1, 8, f) == 8) {
const unsigned char png_sig[8]= {137, 'P', 'N', 'G', 13, 10, 26, 10};
rewind(f);
fclose(f);
if(memcmp(sig, png_sig, 8) == 0) { return load_png(filename, x, y, channels_in_file); }
} else {
fclose(f);
}
return load_bmp(filename, x, y, channels_in_file);
}
void stbi_image_free(void* retval_from_stbi_load) { free(retval_from_stbi_load); }

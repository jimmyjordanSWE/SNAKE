/* stb_image - public domain single-file library for image loading
   Minimal copy for this project: full original is maintained at https://github.com/nothings/stb
   This file contains the public compressed header; the implementation is provided in stb_image.c
*/
#ifndef STB_IMAGE_H
#define STB_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
extern void stbi_image_free(void *retval_from_stbi_load);

#ifdef __cplusplus
}
#endif

#endif /* STB_IMAGE_H */

#ifndef IMG_READER_H
#define IMG_READER_H

#include "libraw/libraw.h"
#include "jpeglib.h"
#include "performancelogger.h"

typedef struct image image;
struct image
{
    unsigned char *data;
    int width;
    int height;
    float *img_grey;
};

image *libraw_rawImage(libraw_data_t *iprc);
image *libraw_thumbImage(libraw_data_t *iprc);
image *process_thumb_image(libraw_processed_image_t *t);

#endif
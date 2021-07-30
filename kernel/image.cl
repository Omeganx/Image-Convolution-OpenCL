
const sampler_t sampler_im = CLK_NORMALIZED_COORDS_FALSE |
                             CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

#define input(p, r) input[3 * (p.y * width + p.x) + r]
#define output(p) output[(p.y * width + p.x)]

#define PI 3.14159265358979
#define GAUSS_NORM 1 / (sqrt(2 * PI))

#define BLUR_SIZE 4

__kernel void preprocess(__global unsigned char *input,
                         __write_only image2d_t dstImg, int width, int height) {
  int2 pos = (int2)(get_global_id(0), get_global_id(1));
  float3 pix =
      ((float3)(input(pos, 0)), (float)(input(pos, 1)), (float)(input(pos, 2)));
  float res = dot(pix, (float3)(0.2989f, 0.587f, 0.114f));
  res = res / 255;
  write_imagef(dstImg, pos, res);
}

__kernel void convolve_gauss_blur_2D_image(__read_only image2d_t srcImg,
                                           __write_only image2d_t dstImag,
                                           int width, int height,
                                           __constant float *filter,
                                           int half_size) {
  int2 pos = {get_global_id(0), get_global_id(1)};
  float sum = 0.0f;

  int2 coord;

    for (int y = 0; y < 2 * half_size + 1; y++) 
      for (int x = 0; x < 2 * half_size + 1; x++)
      {
      coord = (int2)(pos.x + x - half_size, pos.y + y - half_size);
      sum += filter[y * (2 * half_size + 1) + x] *
             read_imagef(srcImg, sampler_im, coord).x;
    }

  write_imagef(dstImag, pos, sum);
}

__kernel void preprocess_buffer(__global unsigned char *input,
                                __global float *output, int width, int height) {
  int2 pos = (int2)(get_global_id(0), get_global_id(1));

  float3 pix =
      ((float3)(input(pos, 0)), (float)(input(pos, 1)), (float)(input(pos, 2)));
  float res = dot(pix, (float3)(0.2989f, 0.587f, 0.114f));
  res = res / 255;
  output(pos) = res;
  barrier(CLK_GLOBAL_MEM_FENCE);
}

__kernel void convolve_gauss_blur_2D(__global float *output,
                                     __global float *image, int width,
                                     int height, __constant float *filter,
                                     int half_size) {
  int2 pos = {get_global_id(0), get_global_id(1)};

  bool border = (pos.x < width - half_size && pos.x > half_size &&
                 pos.y < height - half_size && pos.y > half_size);

  float sum = 0.0;

  if (border) {

  for (int y = 0; y < 2 * half_size + 1; y++)
    for (int x = 0; x < 2 * half_size + 1; x++)
        sum += filter[y * (2 * half_size + 1) + x] *
               image[(pos.y + y - half_size) * width + x + pos.x - half_size];
  }

  output[pos.y * width + pos.x] = sum;
}

__kernel void
convolve_gauss_blur_2D_cache_2(__global float *output, __global float *image,
                               __local float *cache, int width, int height,
                               __constant float *filter, int half_size) {
  int2 pos = {get_global_id(0), get_global_id(1)};
  int2 loc = {get_local_id(0), get_local_id(1)};
  int2 loc_pos = {get_group_id(0), get_group_id(1)};
  int2 size = {get_local_size(0), get_local_size(1)};

  bool border = loc_pos.x == 0 || loc_pos.y == 0 ||
                loc_pos.x == (get_global_size(0) / size.x) - 1 ||
                loc_pos.y == (get_global_size(1) / size.y) - 1;
  if (border)
    return;

  int cache_width = size.x + 2 * half_size;

  int2 cache_coord = {2 * loc.x, 2 * loc.y};
  int2 image_coord =
      cache_coord + loc_pos * size - (int2)(half_size, half_size);

  cache[cache_coord.y * cache_width + cache_coord.x] =
      image[image_coord.y * width + image_coord.x];
  cache[cache_coord.y * cache_width + cache_coord.x + 1] =
      image[image_coord.y * width + image_coord.x + 1];
  cache[(cache_coord.y + 1) * cache_width + cache_coord.x] =
      image[(image_coord.y + 1) * width + image_coord.x];
  cache[(cache_coord.y + 1) * cache_width + cache_coord.x + 1] =
      image[(image_coord.y + 1) * width + image_coord.x + 1];

  barrier(CLK_LOCAL_MEM_FENCE);

  float sum = 0.0f;
  int position;
  int2 offset = {pos.x - loc_pos.x * size.x, pos.y - loc_pos.y * size.y};
  int f_size = 2 * half_size + 1;

  for (int y = 0; y < f_size; y++)
    for (int x = 0; x < f_size; x++)
      sum += filter[y * f_size + x] *
             cache[(offset.y + y) * cache_width + offset.x + x];

  output[pos.y * width + pos.x] = sum;
}

__kernel void convolve_gauss_blur_2D_cache(__global float *output,
                                           __global float *image,
                                           __local float *cache, int width,
                                           int height, __constant float *filter,
                                           int half_size) {
  int2 pos = {get_global_id(0), get_global_id(1)};
  int2 loc = {get_local_id(0), get_local_id(1)};
  int2 loc_pos = {get_group_id(0), get_group_id(1)};
  int2 size = {get_local_size(0), get_local_size(1)};

  bool border = loc_pos.x == 0 || loc_pos.y == 0 ||
                loc_pos.x == (get_global_size(0) / size.x) - 1 ||
                loc_pos.y == (get_global_size(1) / size.y) - 1;
  if (border)
    return;

  /* half_size = 8  || Local size = 16
    (-8, -8) |               | (8, -8)
    --------------------------------
             |(0, 0)         |
             |               |
             |               |
             |      (15, 15) |
    --------------------------------
    (-8, 8)  |               | (8, 8)
  */
  int cache_width = size.x + 2 * half_size;

  cache[(loc.y + half_size) * cache_width + loc.x + half_size] =
      image[pos.y * width + pos.x];

  if (loc.x <= half_size || loc.y <= half_size || loc.x >= size.x - half_size ||
      loc.y >= size.y - half_size) {
    int2 shift = {0, 0};
    if (loc.x <= half_size)
      shift.x = size.x;
    if (loc.y <= half_size)
      shift.y = size.y;
    if (loc.y >= size.y - half_size)
      shift.y = -size.y;
    if (loc.x >= size.x - half_size)
      shift.x = -size.x;

    cache[(loc.y + half_size + shift.y) * cache_width + loc.x + shift.x +
          half_size] = image[(pos.y + shift.y) * width + shift.x + pos.x];
    cache[(loc.y + half_size) * cache_width + loc.x + shift.x + half_size] =
        image[(pos.y) * width + shift.x + pos.x];
    cache[(loc.y + half_size + shift.y) * cache_width + loc.x + half_size] =
        image[(pos.y + shift.y) * width + pos.x];
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  float sum = 0.0f;
  int position;
  int2 offset = {pos.x - loc_pos.x * size.x, pos.y - loc_pos.y * size.y};
  int posi;
  int f_size = 2 * half_size + 1;
  for (int y = 0; y < f_size; y++) {
    posi = (offset.y + y) * cache_width;
    for (int x = 0; x < f_size; x++) {
      // pos_x = x + pos.x - half_size
      // pos_y = y + pos.y - half_size
      // local_pos_x = pos_x - loc_pos.x * size.x;
      // local_pos_y = pos_y - loc_pos.y * size.y;
      // cache_pos_x = local_pos_x + half_size;
      // cache_pos_y = local_pos_y + half_size;
      position = posi + offset.x + x;

      sum += filter[y * f_size + x] * cache[position];
    }
  }
  output[pos.y * width + pos.x] = sum;
}

__kernel void
convolve_gauss_blur_1D_pass1_cache(__global float *output,
                                   __global float *image, __global float *temp,
                                   __local float *cache, int width, int height,
                                   __constant float *filter, int half_size) {

  int2 pos = {get_global_id(0), get_global_id(1)};
  int2 loc = {get_local_id(0), get_local_id(1)};
  int2 size = {get_local_size(0), get_local_size(1)};
  int2 group = {get_group_id(0), get_group_id(1)};
  bool border = (group.x ==0 || group.x == (get_global_size(0) / size.x) - 1);
  if (border) return;

  int f_size = 2 * half_size + 1;

  int cache_coord = 2 * loc.x;
  int image_coord = cache_coord + size.x * group.x - half_size;
  //cache[cache_coord] = image[pos.y * width + image_coord];
  //cache[cache_coord + 1] = image[pos.y * width + image_coord + 1];

  barrier(CLK_LOCAL_MEM_FENCE);
  int offset = pos.x - group.x * size.x;

  float sum = 0.0f;
  for (int x = 0; x < f_size; x++)
    sum += filter[x] * cache[offset + x];

  temp[pos.y * width + pos.x] = sum;
}

__kernel void
convolve_gauss_blur_1D_pass1_cache_nothing(__global float *output,
                                   __global float *image, __global float *temp,
                                   __local float *cache, int width, int height,
                                   __constant float *filter, int half_size) {

  int2 pos = {get_global_id(0), get_global_id(1)};
  
}
__kernel void convolve_gauss_blur_1D_pass1(__global float *output,
                                           __global float *image,
                                           __global float *temp, int width,
                                           int height, __constant float *filter,
                                           int half_size) {
  int2 pos = {get_global_id(0), get_global_id(1)};

  bool border = (pos.x <= half_size || pos.y <= half_size ||
                 pos.y >= height - half_size || pos.x >= width - half_size);
  if (border)
    return;

  int f_size = 2 * half_size + 1;

  float sum = 0.0;
  for (int x = 0; x < f_size; x++)
    sum += filter[x] * image[pos.y * width + pos.x + x - half_size];

  temp[pos.y * width + pos.x] = sum;
  // temp[pos.x * height + pos.y] = sum;
}

__kernel void convolve_gauss_blur_1D_pass2(__global float *output,
                                           __global float *image,
                                           __global float *temp, int width,
                                           int height, __constant float *filter,
                                           int half_size) {
  int2 pos = {get_global_id(0), get_global_id(1)};

  bool border = (pos.x <= half_size || pos.y <= half_size ||
                 pos.y >= height - half_size || pos.x >= width - half_size);
  if (border)
    return;

  int f_size = 2 * half_size + 1;
  float sum = 0.0f;

  for (int y = 0; y < f_size; y++)
    sum += filter[y] * temp[(pos.y + y - half_size) * width + pos.x];
  // sum += filter[y] * temp[(pos.x)*height + pos.y + y - half_size];
  output[pos.y * width + pos.x] = sum;
}
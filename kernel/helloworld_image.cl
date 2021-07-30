
const sampler_t sampler_im = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void im2grey_cl_image(__read_only image2d_t srcImg, __write_only image2d_t dstImg, int width, int height)
{
    int2 imageCoord = (int2) (get_global_id(0), get_global_id(1));
    float4 col = (float4)read_imagef(srcImg, sampler_im, imageCoord);

    float res = 0.2989f*((float)col.x) + 0.5870f*((float)col.y) + 0.1140f*((float)col.z);

    write_imagef(dstImg, imageCoord, res);
}

struct RGB {
    // uint R;
    // uint G;
    // uint B;
    unsigned char R;
    unsigned char G;
    unsigned char B;
    unsigned char align;
};

/*
typedef struct
{

    RGB *data;
    uint size;
    uint height;
    uint weight;

} Image;
*/
__kernel void kernel_histogram(__global struct RGB *pixels, __global unsigned int *R, __global unsigned int *G, __global unsigned int *B) {
    int i = get_global_id(0);
    atomic_add((R + pixels[i].R), 1);
    atomic_add((G + pixels[i].G), 1);
    atomic_add((B + pixels[i].B), 1);
    // barrier(CLK_GLOBAL_MEM_FENCE);
}
/*
__kernel void kernel_histogram(__global Image *img, __global RGB *rgb) {
    // __kernel void kernel_histogram(__global Image *img, __global int *a) {

    //if (get_global_id(0) == 0)
    //atomic_init(&_a, 0);
    //atomic_init(_a, 0);    //initialize variable with zero
    //_a = 0;

    int id = get_global_id(0);
    // rgb[0].R += 1;
    atomic_add(&rgb[img->data[id].R].R, 1);
    // atomic_add(&rgb[img->data[id].G].G, 1);
    // atomic_add(&rgb[img->data[id].B].B, 1);
    barrier(CLK_GLOBAL_MEM_FENCE);

    //atmoic_add(&rgb[0].R, 1);
    // atomic_add(&img->size, 1);
    //char a = img->data[id].R;
    //int r = rgb[0].R;
    //atmoic_inc(&_a);
    //*a = _a;
    //atmoic_add(&a, 1);
    //atmoic_add(&img->weight, 1);
    //a
    // atomic_add(&img->size, 1);
    //atmoic_add(&rgb[0].R);
    //tmoic_add(&(rgb + 0).R, 1);
    //atmoic_add(&rgb[img->data[id].R].R, 1);
    //atmoic_add(&rgb[img->data[id].G]->G, 1);
    //atmoic_add(&rgb[img->data[id].B]->B, 1);
}

*/
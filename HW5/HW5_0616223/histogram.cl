struct RGB
{
    unsigned char R;
    unsigned char G;
    unsigned char B;
    unsigned char align;
};

__kernel void histogram(__global struct RGB *pixel, __global unsigned int *R, __global unsigned int *G, __global unsigned int *B)
{	
	int i = get_global_id(0);
	atomic_add((R + pixel[i].R), 1);
	atomic_add((G + pixel[i].G), 1);
	atomic_add((B + pixel[i].B), 1);
	barrier(CLK_GLOBAL_MEM_FENCE);
}
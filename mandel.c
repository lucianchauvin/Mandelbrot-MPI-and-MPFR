// Created by Lucian Chauvin - me@lucianchauvin.com

#include "mandel.h"

void print_mpfr(mpfr_t op){
        mpfr_out_str(stdout, 10, 0, op, MPFR_RNDD);
        putchar('\n');
}

void print_bounds(mpfr_t bounds [], char* s){
    for(int i = 0; i < 4; ++i){
        printf("%s %d: ", s, i);
        print_mpfr(bounds[i]);
    }
}

// genearte a color map from list of control colors via lerp (should update to cubic-lerp)
void generate_colors(color* colors, size_t ctl_size, color* buf, size_t size){
    int cur_color = 0;
    int block_size = (double) size / (ctl_size - 1);
    for (int c = 0; c < size; c++){
        if(c % block_size == 0 && c != 0) cur_color += 1;
        buf[c] = (color) {(int) (colors[cur_color].r + (((double) (c%block_size))/block_size)*(colors[cur_color+1].r - colors[cur_color].r)),
                          (int) (colors[cur_color].g + (((double) (c%block_size))/block_size)*(colors[cur_color+1].g - colors[cur_color].g)),
                          (int) (colors[cur_color].b + (((double) (c%block_size))/block_size)*(colors[cur_color+1].b - colors[cur_color].b))};
    }
}

int main (int argc, char *argv[]){
    int current_frame = 0;

    color colors [2048];
    generate_colors(blue_gold, 5, colors, 2048);

    mpfr_t center_x, center_y, delta_x, delta_y, factor, skip;
    mpfr_inits2  (PRECISION, center_x, center_y, delta_x, delta_y, factor, skip, NULL);
    mpfr_set_str (center_x, center_x_str, 10, ROUND);
    mpfr_set_str (center_y, center_y_str, 10, ROUND);
    mpfr_set_ui  (skip, SKIP, ROUND);
    mpfr_set_d   (factor, FACTOR, ROUND);

    mpfr_t bounds [4];
    for(int i = 0; i < 4; ++i){
        mpfr_set_zero(bounds[i], 1);
        mpfr_init2(bounds[i], PRECISION);
    }

    mpfr_t w,h;
    mpfr_inits2  (PRECISION, w, h, (mpfr_ptr) 0);

    mpfr_set_d   (w, REAL_OFFSET, ROUND);
    mpfr_set_d   (h, REAL_OFFSET*(HEIGHT/WIDTH), ROUND);

    if(SKIP > 0){
        printf("Skipping %d frames\n", SKIP);
        mpfr_pow (factor, factor, skip, ROUND);
        mpfr_mul (w, w, factor, ROUND);
        mpfr_mul (h, h, factor, ROUND);
    }

    while(current_frame <= MAX_FRAMES){
        mpfr_sub (bounds[0], center_x, w, ROUND); // left
        mpfr_add (bounds[1], center_y, h, ROUND); // top
        mpfr_add (bounds[2], center_x, w, ROUND); // right
        mpfr_sub (bounds[3], center_y, h, ROUND); // bottom

        // x_delta = (bounds[2] - bounds[0])/WIDTH 
        mpfr_sub   (delta_x, bounds[2], bounds[0], ROUND);
        mpfr_div_d (delta_x, delta_x, WIDTH, ROUND);

        // y_delta = (bounds[1] - bounds[3])/HEIGHT 
        mpfr_sub   (delta_y, bounds[1], bounds[3], ROUND);
        mpfr_div_d (delta_y, delta_y, HEIGHT, ROUND);

        unsigned char* dat = malloc(3 * WIDTH * HEIGHT);
        if (!dat) { perror("malloc"); exit(1); }

        double start = omp_get_wtime();

        #pragma omp parallel
        {
            mpfr_t Z_x, Z_y, x2, y2, dist, C_x, C_y;
            mpfr_inits2(PRECISION, Z_x, Z_y, x2, y2, dist, C_x, C_y, NULL);

            #pragma omp for schedule(dynamic)
            for (int v = 0; v < (int)HEIGHT; ++v) {
                // C_y = bounds[3] + (v+1)*y_delta 
                mpfr_mul_ui(C_y, delta_y, (unsigned long)(v + 1), ROUND);
                mpfr_add   (C_y, C_y, bounds[3], ROUND);

                for (int u = 0; u < (int)WIDTH; ++u) {
                    // C_x = bounds[0] + (u+1)*x_delta 
                    mpfr_mul_ui(C_x, delta_x, (unsigned long)(u + 1), ROUND);
                    mpfr_add   (C_x, C_x, bounds[0], ROUND);

                    mpfr_set_zero(Z_x,    1);
                    mpfr_set_zero(Z_y,    1);
                    mpfr_set_zero(x2,   1);
                    mpfr_set_zero(y2,   1);
                    mpfr_set_zero(dist, 1);

                    int iters;
                    for(iters = 0; mpfr_cmp_ui(dist, ESCAPE) <= 0 && iters < MAX_ITERS; ++iters) {
                        mpfr_mul    (Z_y, Z_y, Z_x, ROUND);
                        mpfr_mul_2ui(Z_y, Z_y, 1, ROUND);
                        mpfr_add    (Z_y, Z_y, C_y, ROUND);

                        mpfr_sub(Z_x, x2, y2, ROUND);
                        mpfr_add(Z_x, Z_x, C_x, ROUND);

                        mpfr_sqr(x2, Z_x, ROUND);
                        mpfr_sqr(y2, Z_y, ROUND);

                        mpfr_add(dist, x2, y2, ROUND);
                    }

                    unsigned char* pix = &dat[(v * (int)WIDTH + u) * 3];
                    if (mpfr_cmp_ui(dist, 4) <= 0) {
                        pix[0] = pix[1] = pix[2] = 0;
                    } else {
                        // iters += 1; ?? why is this here
                        mpfr_log2  (dist, dist, ROUND);
                        mpfr_div_d (dist, dist, 2, ROUND);
                        mpfr_log2  (dist, dist, ROUND);
                        int color_idx = ((((int)(sqrt(iters + 10 - mpfr_get_d(dist, ROUND)) * 256) - current_frame * MOV) % 2048) + 2048) % 2048;
                        color c = colors[color_idx];
                        pix[0] = c.r; pix[1] = c.g; pix[2] = c.b;
                    }
                }
            }

            mpfr_clears(Z_x, Z_y, x2, y2, dist, C_x, C_y, NULL);
        }

        char fname[100];
        sprintf(fname, "./frames/%d.ppm", current_frame);

        FILE *fp = fopen(fname, "wb");
        if(fp == NULL){
            printf("failed writing to frames directory");
            exit(1);
        }

        fprintf(fp, "P6\n%d %d\n255\n", (int) WIDTH, (int) HEIGHT);

        (void) fwrite(dat, 1, 3 * (int)WIDTH * (int)HEIGHT, fp);
        free(dat);
        (void) fclose(fp);

        printf("frame %d took %f seconds (%d threads)\n", 
               current_frame, 
               omp_get_wtime() - start, 
               omp_get_max_threads());

        // update zoom
        current_frame += 1;
        mpfr_mul (w, w, factor, ROUND);
        mpfr_mul (h, h, factor, ROUND);
    }
}

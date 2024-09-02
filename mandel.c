/* Created by Lucian Chauvin - lucianchauvin@gmail.gov */

#include "mandel.h"
/* #include <scorep/SCOREP_User.h> */

void print_mpfr(mpfr_t op){
        mpfr_out_str (stdout, 10, 0, op, MPFR_RNDD);
        putchar('\n');
}

void print_bounds(mpfr_t bounds [], char * s){
    for(int i = 0; i < 4; ++i){
        printf("%s %d: ", s, i);
        print_mpfr(bounds[i]);
    }
}

/* Genearte a color map from list of control colors via lerp (should update to cubic-erp */
void generate_colors(color* ctl_colors, size_t ctl_size, color* buf, size_t size){
    int cur_color = 0;
    int block_size = (double) size / (ctl_size - 1);
    for (int c = 0; c < size; c++){
        if(c % block_size == 0 && c != 0) cur_color += 1;
        buf[c] = (color) {(int) (ctl_colors[cur_color].r + (((double) (c%block_size))/block_size)*(ctl_colors[cur_color+1].r - ctl_colors[cur_color].r)),
                          (int) (ctl_colors[cur_color].g + (((double) (c%block_size))/block_size)*(ctl_colors[cur_color+1].g - ctl_colors[cur_color].g)),
                          (int) (ctl_colors[cur_color].b + (((double) (c%block_size))/block_size)*(ctl_colors[cur_color+1].b - ctl_colors[cur_color].b))};
    }
}

int main (int argc, char *argv[]){
    int   n_tasks,               /* total number of tasks in partition */
          rank,                  /* task identifier */
          escape_i,
          current_frame,
          requestor_rank,
          color_i,
          packed_msg_s,
          compute_state   = 0,
          pack_buffer_pos = 0,
          i               = 0,
          j               = 0,
          max_iter        = max_iters;

    FILE* stream;

    char * pack_stream_buffer   = NULL;
    size_t pack_stream_buffer_s = 0;

    char unpack_stream_buffer [1000];
    char pack_buffer [1000];

    color pix_color;

    color colors [2048];
    generate_colors(blue_gold_ctls, 5, colors, 2048);

    time_t seconds;
    clock_t begin, end;
    double time_spent;

    mpfr_t x_center, y_center, x_pix, y_pix, x2, y2, x2y2, x, y, zoom_factor, frame_skip;
    mpfr_inits2  (precision, x_center, y_center, x_pix, y_pix, x, y, x2, y2, x2y2, zoom_factor, frame_skip, (mpfr_ptr) 0);
    mpfr_set_str (x_center, x_center_s, 10, round_mode);
    mpfr_set_str (y_center, y_center_s, 10, round_mode);
    mpfr_set_ui  (frame_skip, frame_skip_init, round_mode);
    mpfr_set_d   (zoom_factor, zoom_init, round_mode);

    MPI_Init(&argc,&argv);

    mpfr_t bounds [4];
    for(i = 0; i < 4; ++i){
        mpfr_set_zero(bounds[i], 1);
        mpfr_init2(bounds[i], precision);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &n_tasks);

    if(n_tasks == 1){
        printf("Please give me more than one process so I can work :3\n");
        MPI_Finalize();
        exit(0);
    }

    MPI_Status status;
    if(rank == PARENT){
        compute_state = 0;
        current_frame = 1;

        mpfr_t w,h;
        mpfr_inits2  (precision, w, h, (mpfr_ptr) 0);

        /* w = x_wid/2; */
        mpfr_set_d   (w, x_wid, round_mode);
        mpfr_div_2ui (w, w, 1, round_mode);

        /* h = x_wid*(rez_height/rez_width)/2; */
        mpfr_set_d   (h, rez_height, round_mode);
        mpfr_div_d   (h, h, rez_width, round_mode);
        mpfr_div_2ui (h, h, 1, round_mode);
        mpfr_mul_d   (h, h, x_wid, round_mode);
        if(frame_skip_init){
            printf("Skipping %d frames\n", frame_skip_init);
            mpfr_pow (zoom_factor, zoom_factor, frame_skip, round_mode);
            mpfr_mul (w, w, zoom_factor, round_mode);
            mpfr_mul (h, h, zoom_factor, round_mode);
            max_iter += frame_skip_init*iter_delta;
        }

        while(current_frame <= max_frames){
            MPI_Recv(&requestor_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            MPI_Send(&compute_state,  1, MPI_INT, requestor_rank, 0, MPI_COMM_WORLD);

            mpfr_sub (bounds[0], x_center, w, round_mode);
            mpfr_add (bounds[1], y_center, h, round_mode);
            mpfr_add (bounds[2], x_center, w, round_mode);
            mpfr_sub (bounds[3], y_center, h, round_mode);

            pack_stream_buffer = NULL;
            pack_stream_buffer_s = 0;
            pack_buffer_pos = 0;
            MPI_Pack(&current_frame, 1, MPI_INT, pack_buffer, 1000, &pack_buffer_pos, MPI_COMM_WORLD);
            MPI_Pack(&max_iter,      1, MPI_INT, pack_buffer, 1000, &pack_buffer_pos, MPI_COMM_WORLD);

            stream = open_memstream(&pack_stream_buffer, &pack_stream_buffer_s); 
            if (stream == NULL) {
                 perror("open_memstream");
                 MPI_Finalize();
                 exit(1);
            }

            for(i = 0; i < 4; ++i)
                mpfr_fpif_export(stream, bounds[i]);

            fclose(stream);
            MPI_Pack (pack_stream_buffer, pack_stream_buffer_s, MPI_BYTE, pack_buffer, 1000, &pack_buffer_pos, MPI_COMM_WORLD);

            MPI_Send (pack_buffer, pack_buffer_pos, MPI_PACKED, requestor_rank, 0, MPI_COMM_WORLD);
            free(pack_stream_buffer);

            current_frame += 1;
            max_iter += iter_delta;
            
            mpfr_mul (w, w, zoom_factor, round_mode);
            mpfr_mul (h, h, zoom_factor, round_mode);
        }

        compute_state = 1;
        for(i = 1; i < n_tasks; ++i){
            MPI_Recv (&requestor_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            MPI_Send (&compute_state,  1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
        //MPI_Bcast(&statusCode, 1, MPI_INT, PARENT, MPI_COMM_WORLD);    why doesnt this work?
        printf("Killing master\n");
        MPI_Finalize();
        exit(0);
    }

    /* Workers */
    MPI_Send(&rank, 1, MPI_INT, PARENT, 0, MPI_COMM_WORLD);
    MPI_Recv(&compute_state, 1, MPI_INT, PARENT, 0, MPI_COMM_WORLD, &status);

    /* keep looping until the parent tells me to die */
    while(!compute_state){
        pack_buffer_pos = 0;
        packed_msg_s = 0;

        MPI_Recv        (pack_buffer, 1000, MPI_PACKED, PARENT, 0, MPI_COMM_WORLD, &status);
        MPI_Get_count   (&status, MPI_BYTE, &packed_msg_s);
        MPI_Unpack      (pack_buffer, 1000, &pack_buffer_pos, &current_frame, 1, MPI_INT, MPI_COMM_WORLD);
        MPI_Unpack      (pack_buffer, 1000, &pack_buffer_pos, &max_iter, 1, MPI_INT, MPI_COMM_WORLD);
        printf("Worker %d recived frame %d\n", rank, current_frame);

        MPI_Unpack      (pack_buffer, 1000, &pack_buffer_pos, unpack_stream_buffer, packed_msg_s - pack_buffer_pos, MPI_BYTE, MPI_COMM_WORLD);
        stream = fmemopen(unpack_stream_buffer, 1000, "r");
        for(i = 0; i < 4; ++i){
            mpfr_fpif_import(bounds[i], stream);
        }
        fclose(stream);

        print_bounds(bounds, "r");

        char fname[100];
        sprintf(fname, "./frames/%d.ppm", current_frame);

        FILE *fp = fopen(fname, "wb");
        if(fp == NULL){
            printf("failed writing to frames directory");
            MPI_Finalize();
            exit(1);
        }
        fprintf(fp, "P6\n%d %d\n255\n", (int) rez_width, (int) rez_height);

        begin = clock();
        for (j = 0; j < (int) rez_height; ++j){
            for (i = 0; i < (int) rez_width; ++i){   
                /* x_pix = bounds[0] + (bounds[2] - bounds[0])*(i/rez_width); */ 
                mpfr_sub   (x_pix, bounds[2], bounds[0], round_mode);
                mpfr_mul_d (x_pix, x_pix, i, round_mode);
                mpfr_div_d (x_pix, x_pix, rez_width, round_mode);
                mpfr_add   (x_pix, x_pix, bounds[0], round_mode);

                /* y_pix = bounds[3] + (bounds[1] - bounds[3])*(j/rez_height); */ 
                mpfr_sub   (y_pix, bounds[1], bounds[3], round_mode);
                mpfr_mul_d (y_pix, y_pix, j, round_mode);
                mpfr_div_d (y_pix, y_pix, rez_height, round_mode);
                mpfr_add   (y_pix, y_pix, bounds[3], round_mode);

                /* x = 0; */
                mpfr_set_ui (x, 0, round_mode);
                /* y = 0; */
                mpfr_set_ui (y, 0, round_mode);
                /* x2 = 0; */
                mpfr_set_ui (x2, 0, round_mode);
                /* y2 = 0; */
                mpfr_set_ui (y2, 0, round_mode);
                /* x2y2 = 0; */
                mpfr_set_ui (x2y2, 0, round_mode);

                /* SCOREP_USER_REGION_DEFINE(inner_loop); */
                /* SCOREP_USER_REGION_BEGIN(inner_loop, "test", SCOREP_USER_REGION_TYPE_LOOP); */

                /* x2 + y2 <= 4 */
                escape_i = 0;
                while (mpfr_cmp_ui(x2y2, escape_radius) <= 0 && escape_i < max_iter){
                    /* y = 2*x*y+yVal; */
                    mpfr_mul     (y, y, x, round_mode);
                    mpfr_mul_2ui (y, y, 1, round_mode);
                    mpfr_add     (y, y, y_pix, round_mode);

                    /* x = x2-y2+xVal; */
                    mpfr_sub     (x, x2, y2, round_mode);
                    mpfr_add     (x, x, x_pix, round_mode);

                    /* x2 = x*x; */
                    mpfr_sqr     (x2, x, round_mode);

                    /* y2 = y*y; */
                    mpfr_sqr     (y2, y, round_mode);
                    mpfr_add     (x2y2, x2, y2, round_mode);

                    escape_i += 1;
                } 
                /* SCOREP_USER_REGION_END(inner_loop); */
                if(mpfr_cmp_ui(x2y2, 4) <= 0){
                    (void) fwrite((char []) {0,0,0}, 1, 3, fp);
                }
                else{
                    escape_i += 1; 
                    mpfr_log2    (x2y2, x2y2, round_mode);
                    mpfr_div_d   (x2y2, x2y2, 2, round_mode);
                    mpfr_log2    (x2y2, x2y2, round_mode);
                    
                    color_i = ((((int)(sqrt(escape_i + 10 - mpfr_get_d(x2y2, round_mode)) * 256) - current_frame*mov_amnt) % 2048) + 2048) % 2048;
                    pix_color = colors[color_i];
                    (void) fwrite((char []){pix_color.r,pix_color.g,pix_color.b}, 1, 3, fp);
                }
            }
        }
        (void) fclose(fp);
        end = clock();
        time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("frame %d took %f seconds\n", current_frame, time_spent);

        /* tell the parent I'm ready for more work */
        MPI_Send(&rank, 1, MPI_INT, PARENT, 0, MPI_COMM_WORLD);
        MPI_Recv(&compute_state, 1, MPI_INT, PARENT, 0, MPI_COMM_WORLD, &status);
    }

    printf("Worker recived kill code, killing\n");
    MPI_Finalize();
    exit(0);
}

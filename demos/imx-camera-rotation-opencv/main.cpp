/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */

#include <wayland-client.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <linux/input-event-codes.h>
#include <opencv2/opencv.hpp>
#include "xdg-shell-client-protocol.h"
#include <mqueue.h>
#include <sys/stat.h>
#include <pthread.h>

using namespace cv;
using namespace std;



//Message Q definitions
#define QUEUE_NAME "/imx-camera-rotation_queue"
#define MAX_SIZE 256
#define MSG_STOP "exit"

//Default values (should be replaced by input arguments)
#define IMAGE_WIDTH    (unsigned int)1280
#define IMAGE_HEIGHT   (unsigned int)720

//Window dimensions
static int width = IMAGE_WIDTH;
static int height = IMAGE_HEIGHT;

//Global variable for angle capture
int angle_deg;

//Wayland globals
struct wl_display *display;
struct wl_compositor *compositor;
struct wl_shm *shm;
struct xdg_wm_base *xdg_wm_base_1;
struct wl_seat *seat;
struct wl_pointer *pointer;
static struct xdg_toplevel *current_toplevel = NULL;
 
//Registry listener to bind Wayland interfaces
static void registry_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = (struct wl_compositor *) wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        shm = (struct wl_shm *) wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        xdg_wm_base_1 = (struct xdg_wm_base *) wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        seat = (struct wl_seat *) wl_registry_bind(registry, name, &wl_seat_interface, 1);
    }
}
 
static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {}
 
//xdg_wm_base listener
static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}
 
static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};
 
//xdg_surface listener
static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    xdg_surface_ack_configure(xdg_surface, serial);
}
 
static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};
 
//xdg_toplevel listener
static void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states) {}
static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
    //Handle window close (e.g., exit)
    exit(0);
}
 
static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
};

//Pointer event handlers
static void pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy) {
    //Pointer entered the surface
}

static void pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface) {
    //Pointer left the surface
}

static void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {
    //Pointer moved within the surface
}

static void pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    if (button == BTN_LEFT) {
        if (state == WL_POINTER_BUTTON_STATE_PRESSED && current_toplevel) {
            //Start window move operation
            xdg_toplevel_move(current_toplevel, seat, serial);
        }
    }
}

static void pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
    //Handle scroll wheel events
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = pointer_axis,
};

//Seat capability handler
static void seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities) {
    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &pointer_listener, NULL);
    }
}

static void seat_name(void *data, struct wl_seat *seat, const char *name) {
    //Seat name
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
    .name = seat_name,
};
 
//Buffer release listener
static void buffer_release(void *data, struct wl_buffer *buffer) {
    //Buffer is free to reuse
}
 
static const struct wl_buffer_listener buffer_listener = {
    .release = buffer_release,
};

void Convert_Rotate(unsigned char* yuvBuffer, int w, int h, unsigned char* rgbaBuffer, int N_angle) {
    Mat M, rotated;
    Point2f center;
    //Black background (B, G, R, A)
    Scalar background_color(0, 0, 0, 0xff); 
    //Adjust angle
    N_angle = 360-(N_angle%360);

    //Create a Mat from the YUV buffer
    //YUYV is 2 bytes per pixel (Y0, U, Y1, V for two pixels), so use CV_8UC2
    Mat yuvImage(h, w, CV_8UC2, (void*)yuvBuffer);
 
    //Convert YUV to RGBA
    Mat rgbaImage;
    cvtColor(yuvImage, rgbaImage, COLOR_YUV2BGRA_YUYV);

    //Rotate frame
    center.x = w/2;
    center.y = h/2;
    M = getRotationMatrix2D(center, N_angle, 1.0);
    warpAffine( rgbaImage, rotated, M, rgbaImage.size(), INTER_LINEAR, BORDER_CONSTANT, background_color);

    //Copy the RGBA data to the output buffer
    if (rgbaBuffer != nullptr) {
        memcpy(rgbaBuffer, rotated.data, w * h * 4);
    }
}


//Structure to pass data to the receiver thread (if needed)
typedef struct {
    mqd_t mq;
} receiver_data_t;
 
//Receiver thread function
void *receiver_thread(void *arg) {
    receiver_data_t *data = (receiver_data_t *)arg;
    mqd_t mq = data->mq;
    char buffer[MAX_SIZE];
    ssize_t bytes_read;
 
    printf("Receiver thread: Waiting for messages...\n");
 
    while (1) {
        //Receive message from queue
        bytes_read = mq_receive(mq, buffer, MAX_SIZE, NULL);
        if (bytes_read == -1) {
            perror("mq_receive");
            break;
        }
 
        buffer[bytes_read] = '\0'; // Null-terminate the string
        angle_deg = atoi(buffer);
        printf("Received angle: %i\n", angle_deg);
 
        //Check if exit message is received
        if (strcmp(buffer, MSG_STOP) == 0) {
            break;
        }
    }
    printf("Receiver thread: Done\n");
    pthread_exit(NULL);
}



/************************ MAIN FUNCTION ******************************/
int main(int argc, char *argv[]) {
    //Verify arguments
    if (argc != 5) {
        printf("Ussage: ./app, v4l2 device, width, height, angle\n");
        return 1;
    }
    
    //Initialize variables with arguments
    char *camera_device = argv[1];    
    width = atoi(argv[2]);
    height = atoi(argv[3]);
    angle_deg = atoi(argv[4]);

    //Adding threads initialization for messageQ
    mqd_t mq;
    struct mq_attr attr;
    pthread_t receiver_tid;
    receiver_data_t receiver_data;
 
    //Initialize queue attributes
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;
 
    //Open the message queue
    mq = mq_open(QUEUE_NAME, O_RDONLY, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        exit(1);
    }
 
    //Prepare data for receiver thread
    receiver_data.mq = mq;
 
    //Create the receiver thread
    if (pthread_create(&receiver_tid, NULL, receiver_thread, &receiver_data) != 0) {
        perror("pthread_create");
        mq_close(mq);
        exit(1);
    }

    //Connect to Wayland display
    display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return 1;
    }
 
    //Set up registry
    struct wl_registry *registry = wl_display_get_registry(display);
    static const struct wl_registry_listener registry_listener = {
        .global = registry_global,
        .global_remove = registry_global_remove,
    };
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);
 
    if (!compositor || !shm || !xdg_wm_base_1 || !seat) {
        fprintf(stderr, "Failed to bind required interfaces\n");
        wl_display_disconnect(display);
        return 1;
    }
 
    xdg_wm_base_add_listener(xdg_wm_base_1, &xdg_wm_base_listener, NULL);
    wl_seat_add_listener(seat, &seat_listener, NULL);
 
    //Open USB camera
    int cam_fd = open(camera_device, O_RDWR);
    if (cam_fd < 0) {
        perror("Failed to open camera");
        wl_display_disconnect(display);
        return 1;
    }
 
    //Configure camera format
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    if (ioctl(cam_fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("Failed to set format");
        close(cam_fd);
        wl_display_disconnect(display);
        return 1;
    }
 
    //Request V4L2 buffers
    struct v4l2_requestbuffers req = {0};
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(cam_fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("Failed to request buffers");
        close(cam_fd);
        wl_display_disconnect(display);
        return 1;
    }
 
    //Map V4L2 buffers
    void *cam_buffers[req.count];
    struct v4l2_buffer buf;
    size_t cam_buffer_lengths[req.count];
    for (unsigned int i = 0; i < req.count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(cam_fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("Failed to query buffer");
            close(cam_fd);
            wl_display_disconnect(display);
            return 1;
        }
        cam_buffers[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, cam_fd, buf.m.offset);
        cam_buffer_lengths[i] = buf.length;
        if (cam_buffers[i] == MAP_FAILED) {
            perror("Failed to mmap buffer");
            close(cam_fd);
            wl_display_disconnect(display);
            return 1;
        }
    }
 
    //Queue V4L2 buffers
    for (unsigned int i = 0; i < req.count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(cam_fd, VIDIOC_QBUF, &buf) < 0) {
            perror("Failed to queue buffer");
            close(cam_fd);
            wl_display_disconnect(display);
            return 1;
        }
    }
 
    //Start streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cam_fd, VIDIOC_STREAMON, &type) < 0) {
        perror("Failed to start streaming");
        close(cam_fd);
        wl_display_disconnect(display);
        return 1;
    }
 
    //Create Wayland shared memory buffer
    int stride = width * 4;
    int size = stride * height;
 
    int shm_fd = memfd_create("wayland-shm", 0);
    if (shm_fd < 0) {
        perror("memfd_create failed");
        close(cam_fd);
        wl_display_disconnect(display);
        return 1;
    }
    if (ftruncate(shm_fd, size) < 0) {
        perror("ftruncate failed");
        close(shm_fd);
        close(cam_fd);
        wl_display_disconnect(display);
        return 1;
    }
 
    void *shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_data == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        close(cam_fd);
        wl_display_disconnect(display);
        return 1;
    }
 
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, shm_fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(shm_fd);
 
    //Add buffer listener
    wl_buffer_add_listener(buffer, &buffer_listener, NULL);
 
    //Create Wayland surface and xdg toplevel
    struct wl_surface *surface = wl_compositor_create_surface(compositor);
    struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base_1, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
    struct xdg_toplevel *xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);
    xdg_toplevel_set_title(xdg_toplevel, "OpenCV Window");
    
    // Set the current toplevel for mouse interaction
    current_toplevel = xdg_toplevel;
    
    wl_surface_commit(surface);
 
    printf("\nInitializations completed (including OpenCV and messageQ),\nentering to the loop...\n");

    //Main loop: capture and display frames
    while (wl_display_dispatch(display) != -1) {

        //Dequeue a frame
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(cam_fd, VIDIOC_DQBUF, &buf) < 0) {
            perror("Failed to dequeue buffer");
            break;
        }
  
        //Requeue the buffer
        if (ioctl(cam_fd, VIDIOC_QBUF, &buf) < 0) {
            perror("Failed to queue buffer");
            break;
        }
       
        //Perform OpenCV conversion
        Convert_Rotate((unsigned char*)cam_buffers[buf.index], width, height, (unsigned char*)shm_data, angle_deg);

        //Update Wayland surface
        wl_surface_attach(surface, buffer, 0, 0);
		wl_surface_damage(surface, 0, 0, width, height);
        wl_surface_commit(surface);
        wl_display_flush(display);
    }
 
    //Wait for the receiver thread to finish
    if (pthread_join(receiver_tid, NULL) != 0) {
        perror("pthread_join");
    }
 
    //Close the queue
    if (mq_close(mq) == -1) {
        perror("mq_close");
        exit(1);
    }

    //Cleanup
    ioctl(cam_fd, VIDIOC_STREAMOFF, &type);
    for (unsigned int i = 0; i < req.count; i++) {
        munmap(cam_buffers[i], cam_buffer_lengths[i]);
    }
    close(cam_fd);
    wl_pointer_destroy(pointer);
    wl_seat_destroy(seat);
    wl_buffer_destroy(buffer);
    munmap(shm_data, size);
    xdg_toplevel_destroy(xdg_toplevel);
    xdg_surface_destroy(xdg_surface);
    wl_surface_destroy(surface);
    xdg_wm_base_destroy(xdg_wm_base_1);
    wl_shm_destroy(shm);
    wl_compositor_destroy(compositor);
    wl_display_disconnect(display);
    return 0;
}
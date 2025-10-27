/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */

#define _GNU_SOURCE
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include <linux/input-event-codes.h>
#include <g2d.h>
#include "xdg-shell-client-protocol.h"
#include <mqueue.h>
#include <sys/stat.h>
#include <pthread.h>



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

//libg2d globals
struct g2d_surface src, dst;
void *g2d_handle;
struct g2d_buf *src_buf, *dst_buf;

//OpenGL globals
GLuint texture, program;
GLint position_attr, texcoord_attr, rotation_uniform,image_size_uniform, window_size_uniform ;
float rotation_angle = 0.0f;

//Global variables for Image data and angle capture
unsigned char *image_data;
int angle_deg;
 
//Global Wayland objects
struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct xdg_wm_base *xdg_wm_base = NULL;
struct wl_surface *surface = NULL;
struct xdg_surface *xdg_surface = NULL;
struct xdg_toplevel *xdg_toplevel = NULL;
struct wl_egl_window *egl_window = NULL;
struct wl_pointer *pointer;
struct wl_seat *seat;
bool moving;
uint32_t pointer_serial;
int32_t pointer_x, pointer_y;
 
//EGL objects
EGLDisplay egl_display;
EGLContext egl_context;
EGLSurface egl_surface;
 
//XDG surface configure handler
static void xdg_surface_handle_configure(void *data, struct xdg_surface *xdg_surface,
                                        uint32_t serial) {
    xdg_surface_ack_configure(xdg_surface, serial);
}
 
//XDG toplevel configure handler
static void xdg_toplevel_handle_configure(void *data, struct xdg_toplevel *xdg_toplevel,
                                         int32_t w, int32_t h, struct wl_array *states) {
    if (w > 0 && h > 0) {
        width = w;
        height = h;
        if (egl_window) {
            wl_egl_window_resize(egl_window, width, height, 0, 0);
        }
    }
}
 
//XDG toplevel close handler
static void xdg_toplevel_handle_close(void *data, struct xdg_toplevel *xdg_toplevel) {
    exit(0);
}

//functions to handle pointer events
static void pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial,
                         struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy) {
    struct window *window = data;
    pointer_serial = serial;
}

static void pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial,
                         struct wl_surface *surface) {
    struct window *window = data;
    moving = false;
}

static void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time,
                          wl_fixed_t sx, wl_fixed_t sy) {
    struct window *window = data;
    pointer_x = wl_fixed_to_int(sx);
    pointer_y = wl_fixed_to_int(sy);
}

static void pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial,
                          uint32_t time, uint32_t button, uint32_t state) {
    struct window *window = data;
    if (button == BTN_LEFT && xdg_toplevel && seat) {
        if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
            xdg_toplevel_move(xdg_toplevel, seat, serial);
            moving = true;
        } else {
            moving = false;
        }
    }
}

//wl_pointer_listeners
static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
};

//seat_capabilities and listener
static void seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities) {
    struct window *window = data;
    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        pointer = wl_seat_get_pointer(seat);
        if (pointer) {
            wl_pointer_add_listener(pointer, &pointer_listener, window);
        } else {
            fprintf(stderr, "Failed to get pointer\n");
        }
    } else if (pointer) {
        wl_pointer_destroy(pointer);
        pointer = NULL;
    }
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
};

//Registry listener to bind Wayland interfaces
static void registry_global(void *data, struct wl_registry *registry, uint32_t name,
                           const char *interface, uint32_t version) {
    struct window *window = data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
        if (seat) {
            wl_seat_add_listener(seat, &seat_listener, window);
        }
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {}

//Listener structs
static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};
 
static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_handle_configure,
};
 
static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_handle_configure,
    .close = xdg_toplevel_handle_close,
};

//Shader compilation
GLuint compile_shader(GLenum type, const char *source) {
   GLuint shader = glCreateShader(type);
   glShaderSource(shader, 1, &source, NULL);
   glCompileShader(shader);
   GLint status;
   glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
   if (!status) {
       char log[512];
       glGetShaderInfoLog(shader, 512, NULL, log);
       fprintf(stderr, "Shader compile error: %s\n", log);
       glDeleteShader(shader);
       return 0;
   }
   fprintf(stderr, "Shader: %x\n", shader);
   return shader;
}

//Initialization of OpenGL shaders and context
int init_gl() {
    const char *vertex_shader_source =  "attribute vec2 position; \n"
                                        "attribute vec2 texcoord; \n"
                                        "varying vec2 v_texcoord; \n"
                                        "uniform float rotation; \n"
                                        "uniform vec2 imageSize; \n"
                                        "uniform vec2 windowSize; \n"
                                        "void main() { \n"
                                        "    vec2 centered = position - imageSize * 0.5; \n"
                                        "    float c = cos(rotation); \n"
                                        "    float s = sin(rotation); \n"
                                        "    mat2 rot = mat2(c, -s, s, c); \n"
                                        "    vec2 rotated = rot * centered; \n"
                                        "    vec2 finalPos = (rotated + imageSize * 0.5) / windowSize * 2.0 - 1.0; \n"
                                        "    gl_Position = vec4(finalPos, 0.0, 1.0); \n"
                                        "    v_texcoord = texcoord; \n"
                                        "} \n";

    const char *fragment_shader_source =    "precision mediump float; \n"
                                            "varying vec2 v_texcoord; \n"
                                            "uniform sampler2D texture; \n"
                                            "void main() {\n"
                                            "    gl_FragColor = texture2D(texture, v_texcoord); \n"
                                            "} \n";

    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    if (!vertex_shader || !fragment_shader) return 0;
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        fprintf(stderr, "Program link error: %s\n", log);
        return 0;
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    position_attr = glGetAttribLocation(program, "position");
    texcoord_attr = glGetAttribLocation(program, "texcoord");
    rotation_uniform = glGetUniformLocation(program, "rotation");

    image_size_uniform = glGetUniformLocation(program, "imageSize");
    window_size_uniform = glGetUniformLocation(program, "windowSize");

    //Create texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return 1;
}

//Draw elements
void GL_render() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);
    glUniform1f(rotation_uniform, rotation_angle);

    glUniform2f(image_size_uniform, (float)width, (float)height);
    glUniform2f(window_size_uniform, (float)width, (float)height);

    GLfloat vertices[] =    {0.0f, 0.0f, 0.0f, 1.0f,
                             width, 0.0f, 1.0f, 1.0f,
                             width, height, 1.0f, 0.0f,
                             0.0f, height, 0.0f, 0.0f };

    glEnableVertexAttribArray(position_attr);
    glVertexAttribPointer(position_attr, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices);
    glEnableVertexAttribArray(texcoord_attr);
    glVertexAttribPointer(texcoord_attr, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices + 2);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(position_attr);
    glDisableVertexAttribArray(texcoord_attr);
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
int main(int argc, char *argv[]) 
{
    //Verify arguments
    if (argc != 5) {
        printf("Ussage: ./app, v4l2 device, width, height, angle\n");
        return 1;
    }
    
    //Initialize variables with arguments
    unsigned char *camera_device = argv[1];    
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

    //Open USB camera
    int cam_fd = open(camera_device, O_RDWR);
    if (cam_fd < 0) {
        perror("Failed to open camera");
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
    
    //Connect to Wayland display
    display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return 1;
    }
 
    //Set up registry and bind globals
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display); // Wait for registry to populate
 
    if (!compositor || !xdg_wm_base) {
        fprintf(stderr, "Failed to bind compositor or xdg_wm_base\n");
        return 1;
    }
 
    //Create surface and XDG toplevel
    surface = wl_compositor_create_surface(compositor);
    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);
    xdg_toplevel_set_title(xdg_toplevel, "OpenGL Window");
    wl_surface_commit(surface);
 
    //Initialize image buffer size
    image_data = malloc(4* width * height);
    
    //Initialize EGL
    egl_display = eglGetDisplay((EGLNativeDisplayType)display);
    if (egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "Failed to get EGL display\n");
        return 1;
    }
    EGLint major, minor;
    if (!eglInitialize(egl_display, &major, &minor)) {
        fprintf(stderr, "Failed to initialize EGL\n");
        return 1;
    }
 
    //Bind OpenGL ES API
    eglBindAPI(EGL_OPENGL_ES_API);
 
    //Choose EGL configuration
    EGLint config_attributes[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    EGLConfig config;
    EGLint num_config;
    eglChooseConfig(egl_display, config_attributes, &config, 1, &num_config);
 
    //Create EGL context
    EGLint context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, context_attributes);
    if (egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "Failed to create EGL context\n");
        return 1;
    }
 
    //Create EGL window surface
    egl_window = wl_egl_window_create(surface, width, height);
    egl_surface = eglCreateWindowSurface(egl_display, config, egl_window, NULL);
    if (egl_surface == EGL_NO_SURFACE) {
        fprintf(stderr, "Failed to create EGL window surface\n");
        return 1;
    }
 
    //Make EGL context current
    if (!eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context)) {
        fprintf(stderr, "Failed to make EGL context current\n");
        return 1;
    }

    //Initialize OpenGL
    if (!init_gl()) {
        fprintf(stderr, "Failed to initialize OpenGL\n");
        return 1;
    }
    glViewport(0, 0, width, height);

    //Initialize G2D
    if (g2d_open(&g2d_handle) != 0) {
        fprintf(stderr, "Failed to open G2D\n");
        return -1;
    }

    //Allocate source and destination buffers
    src_buf = g2d_alloc(width * height * 2, 0);  //src_buf is YUV 16 bpp
    dst_buf = g2d_alloc(width * height * 4, 0);  //dst_buf is RGBA 32 bpp

    if (!src_buf || !dst_buf) {
        fprintf(stderr, "Failed to allocate G2D buffers\n");
        g2d_close(g2d_handle);
        return -1;
    }

    //Configure source surface
    src.format = G2D_YVYU;
    src.planes[0] = src_buf->buf_paddr;
    src.left = 0;
    src.top = 0;
    src.right = width;
    src.bottom = height;
    src.stride = width;
    src.width = width;
    src.height = height;
    src.rot = G2D_ROTATION_0;    
    //Configure destination surface (use SHM buffer for output)
    dst.format = G2D_BGRA8888;
    dst.planes[0] = dst_buf->buf_paddr;  
    dst.left = 0;
    dst.top = 0;
    dst.right = width;
    dst.bottom = height;
    dst.stride = width;
    dst.width = width;
    dst.height = height;

    printf("\nInitializations completed (including OpenGL and messageQ),\nentering to the loop...\n");
    
    //Main loop
    while (1) {

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

        //Copy image data to source buffer
        memcpy(src_buf->buf_vaddr, cam_buffers[buf.index], width * height * 2);

        //Perform G2D blit (rotate into SHM buffer)
        g2d_blit(g2d_handle, &src, &dst);
        g2d_finish(g2d_handle);        

        //Copy image data from destination buffer and regenerate texture        
        memcpy(image_data, dst_buf->buf_vaddr, width * height * 4); 
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);     

        //Update angle and render
        rotation_angle = M_PI*angle_deg/180;
        wl_display_dispatch_pending(display);
        GL_render();
 
        //Swap buffers
        eglSwapBuffers(egl_display, egl_surface);
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
    munmap(shm_data, size);
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(egl_display, egl_surface);
    wl_egl_window_destroy(egl_window);
    eglDestroyContext(egl_display, egl_context);
    eglTerminate(egl_display);
    xdg_toplevel_destroy(xdg_toplevel);
    xdg_surface_destroy(xdg_surface);
    wl_surface_destroy(surface);
    xdg_wm_base_destroy(xdg_wm_base);
    wl_compositor_destroy(compositor);
    wl_display_disconnect(display);
    wl_pointer_destroy(pointer);
    wl_seat_destroy(seat);
    return 0;
}
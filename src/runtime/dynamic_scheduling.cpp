#include "mini_stdint.h"
#include "HalideRuntime.h"

struct tracker {
    size_t min_x, extent_x, min_y, extent_y;
    bool *array;
};

void *create_dynamic_tracker(size_t min_x, size_t extent_x,
                             size_t min_y, size_t extent_y) {
    halide_assert(extent_x >= min_x && extent_y >= min_y);
    size_t xsize = extent_x - min_x;
    size_t ysize = extent_y - min_y;

    // check for overflow in computing size (super lazy way; FIXME: make this better)
    size_t max_size = ((size_t)1) << (sizeof(size_t) * 8 / 2); // sqrt(SIZE_T_MAX+1)
    halide_assert(xsize < max_size && ysize < max_size && "requested size too big");

    bool *array = (bool *)halide_malloc(xsize * ysize);
    halide_assert(array != NULL && "couldn't allocate tracker array");

    // FIXME: is there a memset somewhere?
    for (int i = 0; i < xsize * ysize; i++) {
        array[i] = false;
    }

    struct tracker *t = (struct tracker *)halide_malloc(sizeof(struct tracker));
    halide_assert(t != NULL && "couldn't allocate tracker");
    t->min_x    = min_x;
    t->extent_x = extent_x;
    t->min_y    = min_y;
    t->extent_y = extent_y;

    return (void *)t;
}

void mark_as_computed(void *p, size_t min_x, size_t extent_x,
                               size_t min_y, size_t extent_y) {
    struct tracker *t = (struct tracker *)p;
    halide_assert(min_x >= t->min_x && extent_x <= t->extent_x);
    halide_assert(min_y >= t->min_y && extent_y <= t->extent_y);

    for (size_t j = min_y; j < extent_y; j++) {
        // FIXME: memset
        for (size_t i = min_x; i < extent_x; i++) {
            size_t index = i + (j * (t->extent_x - t->min_x));
            t->array[index] = true;
        }
    }
}

bool need_to_compute(void *p, size_t min_x, size_t extent_x,
                              size_t min_y, size_t extent_y) {
    struct tracker *t = (struct tracker *)p;
    halide_assert(min_x >= t->min_x && extent_x <= t->extent_x);
    halide_assert(min_y >= t->min_y && extent_y <= t->extent_y);

    for (size_t j = min_y; j < extent_y; j++) {
        for (size_t i = min_x; i < extent_x; i++) {
            size_t index = i + (j * (t->extent_x - t->min_x));
            if (!t->array[index]) {
                return true;
            }
        }
    }
    return false;
}

void destroy_dynamic_tracker(void *p) {
    struct tracker *t = (struct tracker *)p;
    halide_free(t->array);
    halide_free(t);
}

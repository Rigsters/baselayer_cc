#ifndef __SWRENDER_H__
#define __SWRENDER_H__


//
// Colors


#define RGBA_BLACK      0, 0, 0, 255
#define RGBA_WHITE      255, 255, 255, 255
#define RGBA_RED        255, 0, 0, 255
#define RGBA_GREEN      0, 255, 0, 255
#define RGBA_BLUE       0, 0, 255, 255
struct Color {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};
void PrintColorInline(Color c) {
    printf("%hhx %hhx %hhx %hhx", c.r, c.g, c.b, c.a);
}


//
// Screen coords


struct Vector2_u16 {
    u16 x;
    u16 y;
};
struct ScreenAnchor {
    u16 x;
    u16 y;
    Color c;
};
inline
bool Cull(u32 pos_x, u32 pos_y, u32 w, u32 h) {
    bool not_result = pos_x >= 0 && pos_x < w && pos_y >= 0 && pos_y < h;
    return !not_result;
}
inline
u32 Pos2Idx(u32 pos_x, u32 pos_y, u32 width) {
    u32 result = pos_x + width * pos_y;
    return result;
}
void ClearToZeroRGBA(u8* image_buffer, u32 w, u32 h) {
    memset(image_buffer, 0, 4 * w * h);
}
inline
Vector2_u16 NDC2Screen(u32 w, u32 h, Vector3f ndc) {
    Vector2_u16 pos;

    pos.x = (u32) ((ndc.x + 1) / 2 * w);
    pos.y = (u32) ((ndc.y + 1) / 2 * h);

    return pos;
}
u16 LinesToScreen(u32 w, u32 h, List<ScreenAnchor> *dest_screen_buffer, List<Vector2_u16> *index_buffer, List<Vector3f> *ndc_buffer, u32 lines_low, u32 lines_high, Color color) {
    u16 nlines_remaining = 0;
    for (u32 i = lines_low; i <= lines_high; ++i) {
        Vector2_u16 line = index_buffer->lst[i];
        Vector2_u16 a = NDC2Screen(w, h, ndc_buffer->lst[line.x]);
        Vector2_u16 b = NDC2Screen(w, h, ndc_buffer->lst[line.y]);

        // TODO: crop to NDC box
        if (Cull(a.x, a.y, w, h) || Cull(b.x, b.y, w, h)) {
            continue;
        }

        *(dest_screen_buffer->lst + dest_screen_buffer->len++) = ScreenAnchor { a.x, a.y, color };
        *(dest_screen_buffer->lst + dest_screen_buffer->len++) = ScreenAnchor { b.x, b.y, color };;
        nlines_remaining++;
    }
    return nlines_remaining;
}


//
// Render to image buffer


void RenderRandomPatternRGBA(u8* image_buffer, u32 w, u32 h) {
    RandInit();
    u32 pix_idx, r, g, b;
    for (u32 i = 0; i < h; ++i) {
        for (u32 j = 0; j < w; ++j) {
            pix_idx = j + i*w;
            r = RandIntMax(255);
            g = RandIntMax(255);
            b = RandIntMax(255);
            image_buffer[4 * pix_idx + 0] = r;
            image_buffer[4 * pix_idx + 1] = g;
            image_buffer[4 * pix_idx + 2] = b;
            image_buffer[4 * pix_idx + 3] = 255;
        }
    }
}
void RenderLineRGBA(u8* image_buffer, u16 w, u16 h, u16 ax, u16 ay, u16 bx, u16 by, Color color) {

    // initially working from a to b
    // there are four cases:
    // 1: slope <= 1, ax < bx
    // 2: slope <= 1, ax > bx 
    // 3: slope > 1, ay < by
    // 4: slope > 1, ay > by 

    f32 slope_ab = (f32) (by - ay) / (bx - ax);

    if (abs(slope_ab) <= 1) {
        // draw by x
        f32 slope = slope_ab;

        // swap?
        if (ax > bx) {
            u16 swapx = ax;
            u16 swapy = ay;

            ax = bx;
            ay = by;
            bx = swapx;
            by = swapy;
        }

        u16 x, y;
        u32 pix_idx;
        for (u16 i = 0; i <= bx - ax; ++i) {
            x = ax + i;
            y = ay + floor(slope * i);

            pix_idx = x + y*w;
            image_buffer[4 * pix_idx + 0] = color.r;
            image_buffer[4 * pix_idx + 1] = color.g;
            image_buffer[4 * pix_idx + 2] = color.b;
            image_buffer[4 * pix_idx + 3] = color.a;
        }
    }
    else {
        // draw by y
        float slope_inv = 1 / slope_ab;

        // swap a & b ?
        if (ay > by) {
            u16 swapx = ax;
            u16 swapy = ay;

            ax = bx;
            ay = by;
            bx = swapx;
            by = swapy;
        }

        u16 x, y;
        u32 pix_idx;
        for (u16 i = 0; i <= by - ay; ++i) {
            y = ay + i;
            x = ax + floor(slope_inv * i);

            pix_idx = x + y*w;
            image_buffer[4 * pix_idx + 0] = color.r;
            image_buffer[4 * pix_idx + 1] = color.g;
            image_buffer[4 * pix_idx + 2] = color.b;
            image_buffer[4 * pix_idx + 3] = color.a;
        }
    }
}
inline
void RenderLineRGBA(u8* image_buffer, u16 w, u16 h, ScreenAnchor a, ScreenAnchor b) {
    RenderLineRGBA(image_buffer, w, h, a.x, a.y, b.x, b.y, a.c);
}
void RenderPointCloud(u8* image_buffer, u16 w, u16 h, Matrix4f *mvp, List<Vector3f> points, Color color) {
    for (u32 i = 0; i < points.len; ++i) {

        Vector3f p_ndc = TransformPerspective( *mvp, points.lst[i] );
        Vector2_u16 p_screen = NDC2Screen(w, h, p_ndc);
        if (Cull(p_screen.x, p_screen.y, w, h) ) {
            continue;
        }
        u32 pix_idx = Pos2Idx(p_screen.x, p_screen.y, w);
        image_buffer[4 * pix_idx + 0] = color.r;
        image_buffer[4 * pix_idx + 1] = color.g;
        image_buffer[4 * pix_idx + 2] = color.b;
        image_buffer[4 * pix_idx + 3] = color.a;
    }
}


//
// SW Rendering thingee


struct SwRenderer {
    // settings
    u32 w;
    u32 h;
    float aspect;
    u32 nchannels;

    // pipeline buffers
    MArena _arena;
    MArena *a;
    List<Vector3f> vertex_buffer;
    List<Vector2_u16> index_buffer;
    List<Vector3f> ndc_buffer;
    List<ScreenAnchor> screen_buffer;
    u8 *image_buffer;
};
SwRenderer InitRenderer(u32 width = 1280, u32 height = 800) {
    SwRenderer rend;
    rend.w = width;
    rend.h = height;
    rend.aspect = (float) width / height;
    u32 nchannels = 4;
    rend._arena = ArenaCreate();
    rend.a = &rend._arena;

    // pipeline buffers
    rend.image_buffer = (u8*) ArenaAlloc(rend.a, nchannels * rend.w * rend.h);
    rend.vertex_buffer = InitList<Vector3f>(rend.a, 1000);
    rend.index_buffer = InitList<Vector2_u16>(rend.a, 1000);
    rend.ndc_buffer = InitList<Vector3f>(rend.a, 1000);
    rend.screen_buffer = InitList<ScreenAnchor>(rend.a, 1000);

    return rend;
}


//
// Entity types

enum EntityType {
    ET_AXES,
    ET_LINES,
    ET_POINTCLOUD,

    ET_COUNT,
};
struct Entity {
    Entity *next = NULL;
    EntityType tpe;
    Matrix4f transform;
    Color color { RGBA_WHITE };
    u16 verts_low = 0;
    u16 verts_high = 0;
    u16 lines_low = 0;
    u16 lines_high = 0;
};
Entity InitEntity(EntityType tpe) {
    Entity e;
    e.tpe = tpe;
    e.transform = Matrix4f_Identity();
    return e;
}
struct EntitySystem {
    Entity *first = NULL;
    Entity *last = NULL;
};
void EntitySteysmChain(EntitySystem *es, Entity *next) {
    if (es->first == NULL) {
        es->first = next;
        es->last = next;
    }
    else {
        es->last->next = next;
        es->last = next;
    }
}


// Axis-aligned box
struct AABox {
    Entity _entity;
    Vector3f center_loc;
    f32 radius;
};
AABox InitAABox(Vector3f center_transf, float radius) {
    AABox box;
    box._entity = InitEntity(ET_LINES);
    box._entity.transform = TransformBuild(y_hat, 0, center_transf);
    box._entity.color = { RGBA_GREEN };
    box.center_loc = Vector3f {0, 0, 0},
    box.radius = radius;
    return box;
}
void AABoxGetVerticesAndIndices(AABox *box, List<Vector3f> *verts_dest, List<Vector2_u16> *idxs_dest) {
    u16 vertex_offset = verts_dest->len;
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset + 0), (u16) (vertex_offset + 1) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset + 0), (u16) (vertex_offset + 2) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset + 0), (u16) (vertex_offset + 4) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset + 3), (u16) (vertex_offset + 1) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset + 3), (u16) (vertex_offset + 2) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset + 3), (u16) (vertex_offset + 7) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset + 5), (u16) (vertex_offset + 1) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset + 5), (u16) (vertex_offset + 4) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset + 5), (u16) (vertex_offset + 7) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset + 6), (u16) (vertex_offset + 2) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset + 6), (u16) (vertex_offset + 4) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset + 6), (u16) (vertex_offset + 7) };

    *(verts_dest->lst + verts_dest->len++) = Vector3f { box->center_loc.x - box->radius, box->center_loc.y - box->radius, box->center_loc.z - box->radius };
    *(verts_dest->lst + verts_dest->len++) = Vector3f { box->center_loc.x - box->radius, box->center_loc.y - box->radius, box->center_loc.z + box->radius };
    *(verts_dest->lst + verts_dest->len++) = Vector3f { box->center_loc.x - box->radius, box->center_loc.y + box->radius, box->center_loc.z - box->radius };
    *(verts_dest->lst + verts_dest->len++) = Vector3f { box->center_loc.x - box->radius, box->center_loc.y + box->radius, box->center_loc.z + box->radius };
    *(verts_dest->lst + verts_dest->len++) = Vector3f { box->center_loc.x + box->radius, box->center_loc.y - box->radius, box->center_loc.z - box->radius };
    *(verts_dest->lst + verts_dest->len++) = Vector3f { box->center_loc.x + box->radius, box->center_loc.y - box->radius, box->center_loc.z + box->radius };
    *(verts_dest->lst + verts_dest->len++) = Vector3f { box->center_loc.x + box->radius, box->center_loc.y + box->radius, box->center_loc.z - box->radius };
    *(verts_dest->lst + verts_dest->len++) = Vector3f { box->center_loc.x + box->radius, box->center_loc.y + box->radius, box->center_loc.z + box->radius };
}
void AABoxActivate(AABox *box, SwRenderer *r) {
    box->_entity.verts_low = r->vertex_buffer.len;
    box->_entity.lines_low = r->index_buffer.len;
    AABoxGetVerticesAndIndices(box, &r->vertex_buffer, &r->index_buffer);
    box->_entity.verts_high = r->vertex_buffer.len - 1;
    box->_entity.lines_high = r->index_buffer.len - 1;
}


// Coordinate axes
struct CoordAxes {
    Entity _entity;
    Vector3f x { 1, 0, 0 };
    Vector3f y { 0, 1, 0 };
    Vector3f z { 0, 0, 1 };
    Vector3f origo { 0, 0, 0 };
};
CoordAxes InitCoordAxes() {
    CoordAxes ax;
    ax._entity = InitEntity(ET_AXES);
    ax._entity.color = { RGBA_BLUE };
    return ax;
}
void CoordAxesGetVerticesAndIndices(CoordAxes *axes, List<Vector3f> *verts_dest, List<Vector2_u16> *idxs_dest) {
    u16 vertex_offset = verts_dest->len;
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset), (u16) (vertex_offset + 1) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset), (u16) (vertex_offset + 2) };
    *(idxs_dest->lst + idxs_dest->len++) = Vector2_u16 { (u16) (vertex_offset), (u16) (vertex_offset + 3) };
    
    *(verts_dest->lst + verts_dest->len++) = (axes->origo);
    *(verts_dest->lst + verts_dest->len++) = (axes->origo + axes->x);
    *(verts_dest->lst + verts_dest->len++) = (axes->origo + axes->y);
    *(verts_dest->lst + verts_dest->len++) = (axes->origo + axes->z);
}
void CoordAxesActivate(CoordAxes *axes, SwRenderer *r) {
    axes->_entity.verts_low = r->vertex_buffer.len;
    axes->_entity.lines_low = r->index_buffer.len;
    CoordAxesGetVerticesAndIndices(axes, &r->vertex_buffer, &r->index_buffer);
    axes->_entity.verts_high = r->vertex_buffer.len - 1;
    axes->_entity.lines_high = r->index_buffer.len - 1;
}


// Point cloud
struct PointCloud {
    Entity _entity;
    List<Vector3f> points;
};
PointCloud InitPointCloud(List<Vector3f> points) {
    PointCloud pc;
    pc._entity = InitEntity(ET_POINTCLOUD);
    pc.points = points;
    return pc;
}
PointCloud InitPointCloud(Vector3f *points, u32 npoints) {
    return InitPointCloud( List<Vector3f> { points, npoints });
}
PointCloud InitPointCloudF(float *points, u32 npoints) {
    return InitPointCloud((Vector3f*) points, npoints);
}


#endif

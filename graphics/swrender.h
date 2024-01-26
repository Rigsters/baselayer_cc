#ifndef __SWRENDER_H__
#define __SWRENDER_H__


#include "../baselayer.h"
#include "geometry.h"
#include "entity.h"
#include "shaders.h"


//
// Screen coords


struct ScreenAnchor {
    s16 x;
    s16 y;
    Color c;
};
inline
bool CullScreenCoords(u32 pos_x, u32 pos_y, u32 w, u32 h) {
    bool not_result = pos_x >= 0 && pos_x < w && pos_y >= 0 && pos_y < h;
    return !not_result;
}
inline
u32 ScreenCoordsPosition2Idx(u32 pos_x, u32 pos_y, u32 width) {
    u32 result = pos_x + width * pos_y;
    return result;
}
inline
Vector2_s16 NDC2Screen(u32 w, u32 h, Vector3f ndc) {
    Vector2_s16 pos;

    pos.x = (s16) ((ndc.x + 1) / 2 * w);
    pos.y = (s16) ((ndc.y + 1) / 2 * h);

    return pos;
}
u16 LinesToScreenCoords(u32 w, u32 h, List<ScreenAnchor> *dest_screen_buffer, List<Vector2_u16> *index_buffer, List<Vector3f> *ndc_buffer, u32 lines_low, u32 lines_high, Color color) {
    u16 nlines_remaining = 0;
    for (u32 i = lines_low; i <= lines_high; ++i) {
        Vector2_u16 line = index_buffer->lst[i];
        Vector2_s16 a = NDC2Screen(w, h, ndc_buffer->lst[line.x]);
        Vector2_s16 b = NDC2Screen(w, h, ndc_buffer->lst[line.y]);

        if (CullScreenCoords(a.x, a.y, w, h) && CullScreenCoords(b.x, b.y, w, h)) {
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
void RenderLineRGBA(u8* image_buffer, u16 w, u16 h, s16 ax, s16 ay, s16 bx, s16 by, Color color) {

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

        s16 x, y;
        u32 pix_idx;
        for (u16 i = 0; i <= bx - ax; ++i) {
            x = ax + i;
            y = ay + floor(slope * i);

            if (CullScreenCoords(x, y, w, h)) {
                continue;
            }

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

        s16 x, y;
        u32 pix_idx;
        for (u16 i = 0; i <= by - ay; ++i) {
            y = ay + i;
            x = ax + floor(slope_inv * i);

            if (CullScreenCoords(x, y, w, h)) {
                continue;
            }

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
    float color_fade = 1;
    Vector3f p;
    Vector3f p_ndc;
    Vector2_s16 p_screen;
    u32 pix_idx;
    for (u32 i = 0; i < points.len; ++i) {
        p = points.lst[i];
        p_ndc = TransformPerspective( *mvp, p );
        p_screen = NDC2Screen(w, h, p_ndc);
        if (CullScreenCoords(p_screen.x, p_screen.y, w, h) ) {
            continue;
        }
        pix_idx = ScreenCoordsPosition2Idx(p_screen.x, p_screen.y, w);
        image_buffer[4 * pix_idx + 0] = color.r * color_fade;
        image_buffer[4 * pix_idx + 1] = color.g * color_fade;
        image_buffer[4 * pix_idx + 2] = color.b * color_fade;
        image_buffer[4 * pix_idx + 3] = color.a;
    }
}
#define NORMALS_DOWNSCALE_FACTOR 0.01f
void RenderPointCloudWithNormals(u8* image_buffer, u16 w, u16 h, Matrix4f *mvp, List<Vector3f> points, List<Vector3f> normals, Color color, Color color_normals) {
    assert(points.len == normals.len);

    Vector3f p, n2; // normal from [n1 == p] -> [n2]
    Vector3f p_ndc, n2_ndc;
    Vector2_s16 p_screen;
    u32 pix_idx;
    for (u32 i = 0; i < points.len; ++i) {

        p = points.lst[i];
        p_ndc = TransformPerspective( *mvp, p );
        p_screen = NDC2Screen(w, h, p_ndc);
        if (CullScreenCoords(p_screen.x, p_screen.y, w, h) ) {
            continue;
        }

        n2 = p + NORMALS_DOWNSCALE_FACTOR * normals.lst[i];
        n2_ndc = TransformPerspective( *mvp, n2 );
        Vector2_s16 p_screen = NDC2Screen(w, h, p_ndc);
        Vector2_s16 n2_screen = NDC2Screen(w, h, n2_ndc);

        // render point
        RenderLineRGBA(image_buffer, w, h, p_screen.x, p_screen.y, n2_screen.x, n2_screen.y, color_normals);

        // render point on top of normal
        pix_idx = ScreenCoordsPosition2Idx(p_screen.x, p_screen.y, w);
        image_buffer[4 * pix_idx + 0] = color.r;
        image_buffer[4 * pix_idx + 1] = color.g;
        image_buffer[4 * pix_idx + 2] = color.b;
        image_buffer[4 * pix_idx + 3] = color.a;
    }
}
void RenderMesh() {
    printf("RenderMesh: not implemented\n");
}


//
// 2D blit


void BlitImageInvertY(ImageRGBA dest, ImageRGB src, Rect blit_in) {
    Rect rect_dest = InitRectangle(dest.width, dest.height);
    Rect blit = RectangleCrop(rect_dest, blit_in);

    assert(blit_in.left == blit.left && blit_in.top == blit.top && blit_in.width == blit.width && blit_in.height == blit.height);

    // point-sample source
    float scale_x = (float) src.width / blit_in.width;
    float scale_y = (float) src.height / blit_in.height;

    u32 k, l; // source indices
    u32 ii; // dest y index
    u32 idx_dest;
    u32 idx_src;

    // looping on rectangle coordinates
    for (u32 i = 0; i < blit.height; ++i) {
        for (u32 j = 0; j < blit.width; ++j) {
            //ii = i + blit.top; // <- not inverted
            ii = dest.height - 1 - (i + blit.top); // <- inverted
            idx_dest = ii*dest.width + j + blit.left;

            k = floor( scale_y * i ); // <- not inverted 
            //k = src.height - 1 - floor( scale_y * i ); // <- inverted
            l = floor( scale_x * j ); 
            idx_src = k*src.width + l;

            dest.img[idx_dest] = * (Color*) (src.img + idx_src * 3);
            (dest.img + idx_dest)->a = 255;
        }
    }
}


//
// Rendering functions wrapper


struct SwRenderer {
    bool initialized;

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

    // shader
    ScreenQuadTextureProgram screen;
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
    // TODO: These should be "chunked" lists - list chunks linked by a next*
    rend.image_buffer = (u8*) ArenaAlloc(rend.a, nchannels * rend.w * rend.h);
    rend.vertex_buffer = InitList<Vector3f>(rend.a, 1024 * 8);
    rend.index_buffer = InitList<Vector2_u16>(rend.a, 1024 * 8);
    rend.ndc_buffer = InitList<Vector3f>(rend.a, 1024 * 8);
    rend.screen_buffer = InitList<ScreenAnchor>(rend.a, 1024 * 8);

    // shader
    rend.screen.Init();
    rend.initialized = true;

    return rend;
}
void FreeRenderer(SwRenderer *rend) {
    // TODO: impl.
}


// TODO: sort out static/re-usable memory
void SwRenderFrame(SwRenderer *r, EntitySystem *es, Matrix4f *vp, u64 frameno) {
    TimeFunction;

    _memzero(r->image_buffer, 4 * r->w * r->h);
    r->screen_buffer.len = 0;

    // entity loop (POC): vertices -> NDC
    Matrix4f model, mvp;
    u32 eidx = 0;
    TreeIterState iter;
    InitTreeIter(&iter);
    Entity *next = NULL;
    while ((next = es->TreeIterNext(next, &iter)) != NULL) {

        if (next->active) {

            if (EntityIsTiedToRenderer(next->tpe)) {
                if (next->tpe == ET_LINES_ROT) {
                    model = next->transform * TransformBuildRotateY(0.03f * frameno);
                }
                else {
                    model = next->transform;
                }
                mvp = TransformBuildMVP(model, *vp);

                // lines to screen buffer
                for (u32 i = next->verts_low; i <= next->verts_high; ++i) {
                    r->ndc_buffer.lst[i] = TransformPerspective(mvp, r->vertex_buffer.lst[i]);
                    // TODO: here, it is faster to do a frustum filter in world space
                }
                // render lines
                LinesToScreenCoords(r->w, r->h, &r->screen_buffer, &r->index_buffer, &r->ndc_buffer, next->lines_low, next->lines_high, next->color);
            }
            else {
                mvp = TransformBuildMVP(next->transform, *vp);

                // render pointcloud
                if (next->tpe == ET_POINTCLOUD) {
                    RenderPointCloud(r->image_buffer, r->w, r->h, &mvp, next->GetVertices(), next->color);
                }
                else if (next->tpe == ET_POINTCLOUD_W_NORMALS) {
                    RenderPointCloudWithNormals(r->image_buffer, r->w, r->h, &mvp, next->GetVertices(), next->GetNormals(), next->color, next->color_alt);
                }
                else if (next->tpe == ET_MESH) {
                    RenderMesh();
                    // as a fallback, just render the vertices as a point cloud:
                    RenderPointCloud(r->image_buffer, r->w, r->h, &mvp, next->GetVertices(), next->color);
                }
                else if (next->tpe == ET_BLITBOX_RGB) {
                    assert(next->ext_texture != NULL);
                    Rect blitbox = next->GetBlitBox();

                    // TODO: remove hack
                    ImageRGBA _img_src = next->GetTexture();
                    ImageRGB img_src { _img_src.width, _img_src.height, (u8*) _img_src.img };
                    // TODO: change the image buffer to be of type Color* or even ImageRGBA*
                    ImageRGBA img_dest { r->w, r->h, (Color*) r->image_buffer };
                    BlitImageInvertY(img_dest, img_src, blitbox);
                }
                else if (next->tpe == ET_BLITBOX_RGBA) {
                    assert(1 == 0 && "not implemented");
                }
                else if (next->tpe == ET_EMPTY_NODE) {
                    // just an iteration handle
                }
                else {
                    printf("Unknown entity type: %d (stream/external)\n", next->tpe);
                }
            }
        }
        eidx++;
    }
    r->ndc_buffer.len = r->vertex_buffer.len;
    for (u32 i = 0; i < r->screen_buffer.len / 2; ++i) {
        RenderLineRGBA(r->image_buffer, r->w, r->h, r->screen_buffer.lst[2*i + 0], r->screen_buffer.lst[2*i + 1]);
    }

    r->screen.Draw(r->image_buffer, r->w, r->h);
}


Entity *EntityAABox(EntitySystem *es, Entity* branch, Vector3f center_transf, float radius, SwRenderer *r) {
    Entity *dest = es->AllocEntityChild(branch);
    Entity box = AABox(center_transf, radius, &r->vertex_buffer, &r->index_buffer);
    EntityCopyBodyOnly(dest, box);
    return dest;
}


Entity *EntityCoordAxes(EntitySystem *es, Entity* branch, SwRenderer *r) {
    Entity *dest = es->AllocEntityChild(branch);
    Entity axes = CoordAxes(&r->vertex_buffer, &r->index_buffer);
    EntityCopyBodyOnly(dest, axes);
    return dest;
}


Entity *EntityCameraWireframe(EntitySystem *es, Entity* branch, float size = 0.05f, SwRenderer *r = NULL) {
    assert(r != NULL);
    Entity *dest = es->AllocEntityChild(branch);
    float radius_xy = 0.5 * size;
    float length_z = 1.5 * size;
    Entity cam = CameraWireframe(radius_xy, length_z, &r->vertex_buffer, &r->index_buffer);
    EntityCopyBodyOnly(dest, cam);
    return dest;
}


#endif

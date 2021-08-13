/*
 * GDI mapping mode functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ntgdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dc);


static SIZE get_dc_virtual_size( DC *dc )
{
    SIZE ret = dc->attr->virtual_size;

    if (!ret.cx)
    {
        ret.cx = GetDeviceCaps( dc->hSelf, HORZSIZE );
        ret.cy = GetDeviceCaps( dc->hSelf, VERTSIZE );
    }
    return ret;
}

static SIZE get_dc_virtual_res( DC *dc )
{
    SIZE ret = dc->attr->virtual_res;

    if (!ret.cx)
    {
        ret.cx = GetDeviceCaps( dc->hSelf, HORZRES );
        ret.cy = GetDeviceCaps( dc->hSelf, VERTRES );
    }
    return ret;
}

/***********************************************************************
 *           MAPPING_FixIsotropic
 *
 * Fix viewport extensions for isotropic mode.
 */
static void MAPPING_FixIsotropic( DC * dc )
{
    SIZE virtual_size = get_dc_virtual_size( dc );
    SIZE virtual_res = get_dc_virtual_res( dc );
    double xdim = fabs((double)dc->attr->vport_ext.cx * virtual_size.cx /
                       (virtual_res.cx * dc->attr->wnd_ext.cx));
    double ydim = fabs((double)dc->attr->vport_ext.cy * virtual_size.cy /
                       (virtual_res.cy * dc->attr->wnd_ext.cy));

    if (xdim > ydim)
    {
        INT mincx = (dc->attr->vport_ext.cx >= 0) ? 1 : -1;
        dc->attr->vport_ext.cx = floor(dc->attr->vport_ext.cx * ydim / xdim + 0.5);
        if (!dc->attr->vport_ext.cx) dc->attr->vport_ext.cx = mincx;
    }
    else
    {
        INT mincy = (dc->attr->vport_ext.cy >= 0) ? 1 : -1;
        dc->attr->vport_ext.cy = floor(dc->attr->vport_ext.cy * xdim / ydim + 0.5);
        if (!dc->attr->vport_ext.cy) dc->attr->vport_ext.cy = mincy;
    }
}


BOOL set_map_mode( DC *dc, int mode )
{
    SIZE virtual_size, virtual_res;

    if (mode == dc->attr->map_mode && (mode == MM_ISOTROPIC || mode == MM_ANISOTROPIC))
        return TRUE;

    virtual_size = get_dc_virtual_size( dc );
    virtual_res = get_dc_virtual_res( dc );
    switch (mode)
    {
    case MM_TEXT:
        dc->attr->wnd_ext.cx   = 1;
        dc->attr->wnd_ext.cy   = 1;
        dc->attr->vport_ext.cx = 1;
        dc->attr->vport_ext.cy = 1;
        break;
    case MM_LOMETRIC:
    case MM_ISOTROPIC:
        dc->attr->wnd_ext.cx   = virtual_size.cx * 10;
        dc->attr->wnd_ext.cy   = virtual_size.cy * 10;
        dc->attr->vport_ext.cx = virtual_res.cx;
        dc->attr->vport_ext.cy = -virtual_res.cy;
        break;
    case MM_HIMETRIC:
        dc->attr->wnd_ext.cx   = virtual_size.cx * 100;
        dc->attr->wnd_ext.cy   = virtual_size.cy * 100;
        dc->attr->vport_ext.cx = virtual_res.cx;
        dc->attr->vport_ext.cy = -virtual_res.cy;
        break;
    case MM_LOENGLISH:
        dc->attr->wnd_ext.cx   = MulDiv(1000, virtual_size.cx, 254);
        dc->attr->wnd_ext.cy   = MulDiv(1000, virtual_size.cy, 254);
        dc->attr->vport_ext.cx = virtual_res.cx;
        dc->attr->vport_ext.cy = -virtual_res.cy;
        break;
    case MM_HIENGLISH:
        dc->attr->wnd_ext.cx   = MulDiv(10000, virtual_size.cx, 254);
        dc->attr->wnd_ext.cy   = MulDiv(10000, virtual_size.cy, 254);
        dc->attr->vport_ext.cx = virtual_res.cx;
        dc->attr->vport_ext.cy = -virtual_res.cy;
        break;
    case MM_TWIPS:
        dc->attr->wnd_ext.cx   = MulDiv(14400, virtual_size.cx, 254);
        dc->attr->wnd_ext.cy   = MulDiv(14400, virtual_size.cy, 254);
        dc->attr->vport_ext.cx = virtual_res.cx;
        dc->attr->vport_ext.cy = -virtual_res.cy;
        break;
    case MM_ANISOTROPIC:
        break;
    default:
        return FALSE;
    }
    /* RTL layout is always MM_ANISOTROPIC */
    if (!(dc->attr->layout & LAYOUT_RTL)) dc->attr->map_mode = mode;
    DC_UpdateXforms( dc );
    return TRUE;
}

BOOL CDECL nulldrv_ModifyWorldTransform( PHYSDEV dev, const XFORM *xform, DWORD mode )
{
    DC *dc = get_nulldrv_dc( dev );

    switch (mode)
    {
    case MWT_IDENTITY:
        dc->xformWorld2Wnd.eM11 = 1.0f;
        dc->xformWorld2Wnd.eM12 = 0.0f;
        dc->xformWorld2Wnd.eM21 = 0.0f;
        dc->xformWorld2Wnd.eM22 = 1.0f;
        dc->xformWorld2Wnd.eDx  = 0.0f;
        dc->xformWorld2Wnd.eDy  = 0.0f;
        break;
    case MWT_LEFTMULTIPLY:
        CombineTransform( &dc->xformWorld2Wnd, xform, &dc->xformWorld2Wnd );
        break;
    case MWT_RIGHTMULTIPLY:
        CombineTransform( &dc->xformWorld2Wnd, &dc->xformWorld2Wnd, xform );
        break;
    default:
        return FALSE;
    }
    DC_UpdateXforms( dc );
    return TRUE;
}

BOOL CDECL nulldrv_SetWorldTransform( PHYSDEV dev, const XFORM *xform )
{
    DC *dc = get_nulldrv_dc( dev );

    dc->xformWorld2Wnd = *xform;
    DC_UpdateXforms( dc );
    return TRUE;
}

/***********************************************************************
 *           dp_to_lp
 *
 * Internal version of DPtoLP that takes a DC *.
 */
BOOL dp_to_lp( DC *dc, POINT *points, INT count )
{
    if (dc->vport2WorldValid)
    {
        while (count--)
        {
            double x = points->x;
            double y = points->y;
            points->x = floor( x * dc->xformVport2World.eM11 +
                               y * dc->xformVport2World.eM21 +
                               dc->xformVport2World.eDx + 0.5 );
            points->y = floor( x * dc->xformVport2World.eM12 +
                               y * dc->xformVport2World.eM22 +
                               dc->xformVport2World.eDy + 0.5 );
            points++;
        }
    }
    return (count < 0);
}

/***********************************************************************
 *           NtGdiTransformPoints    (win32u.@)
 */
BOOL WINAPI NtGdiTransformPoints( HDC hdc, const POINT *points_in, POINT *points_out,
                                  INT count, UINT mode )
{
    DC *dc = get_dc_ptr( hdc );
    int i = 0;
    BOOL ret = FALSE;

    if (!dc) return FALSE;

    switch (mode)
    {
    case NtGdiLPtoDP:
        for (i = 0; i < count; i++)
        {
            double x = points_in[i].x;
            double y = points_in[i].y;
            points_out[i].x = floor( x * dc->xformWorld2Vport.eM11 +
                                     y * dc->xformWorld2Vport.eM21 +
                                     dc->xformWorld2Vport.eDx + 0.5 );
            points_out[i].y = floor( x * dc->xformWorld2Vport.eM12 +
                                     y * dc->xformWorld2Vport.eM22 +
                                     dc->xformWorld2Vport.eDy + 0.5 );
        }
        ret = TRUE;
        break;

    case NtGdiDPtoLP:
        if (!dc->vport2WorldValid) break;
        for (i = 0; i < count; i++)
        {
            double x = points_in[i].x;
            double y = points_in[i].y;
            points_out[i].x = floor( x * dc->xformVport2World.eM11 +
                                     y * dc->xformVport2World.eM21 +
                                     dc->xformVport2World.eDx + 0.5 );
            points_out[i].y = floor( x * dc->xformVport2World.eM12 +
                                     y * dc->xformVport2World.eM22 +
                                     dc->xformVport2World.eDy + 0.5 );
        }
        ret = TRUE;
        break;

    default:
        WARN( "invalid mode %x\n", mode );
        break;
    }

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           lp_to_dp
 *
 * Internal version of LPtoDP that takes a DC *.
 */
void lp_to_dp( DC *dc, POINT *points, INT count )
{
    while (count--)
    {
        double x = points->x;
        double y = points->y;
        points->x = floor( x * dc->xformWorld2Vport.eM11 +
                           y * dc->xformWorld2Vport.eM21 +
                           dc->xformWorld2Vport.eDx + 0.5 );
        points->y = floor( x * dc->xformWorld2Vport.eM12 +
                           y * dc->xformWorld2Vport.eM22 +
                           dc->xformWorld2Vport.eDy + 0.5 );
        points++;
    }
}


/***********************************************************************
 *           NtGdiComputeXformCoefficients    (win32u.@)
 */
BOOL WINAPI NtGdiComputeXformCoefficients( HDC hdc )
{
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    if (dc->attr->map_mode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
    DC_UpdateXforms( dc );
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           NtGdiScaleViewportExtEx    (win32u.@)
 */
BOOL WINAPI NtGdiScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom,
                                     INT y_num, INT y_denom, SIZE *size )
{
    DC *dc;

    if ((!(dc = get_dc_ptr( hdc )))) return FALSE;

    if (size) *size = dc->attr->vport_ext;

    if (dc->attr->map_mode == MM_ISOTROPIC || dc->attr->map_mode == MM_ANISOTROPIC)
    {
        if (!x_num || !x_denom || !y_num || !y_denom)
        {
            release_dc_ptr( dc );
            return FALSE;
        }

        dc->attr->vport_ext.cx = (dc->attr->vport_ext.cx * x_num) / x_denom;
        dc->attr->vport_ext.cy = (dc->attr->vport_ext.cy * y_num) / y_denom;
        if (dc->attr->vport_ext.cx == 0) dc->attr->vport_ext.cx = 1;
        if (dc->attr->vport_ext.cy == 0) dc->attr->vport_ext.cy = 1;
        if (dc->attr->map_mode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
        DC_UpdateXforms( dc );
    }

    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           NtGdiScaleWindowExtEx    (win32u.@)
 */
BOOL WINAPI NtGdiScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom,
                                   INT y_num, INT y_denom, SIZE *size )
{
    DC *dc;

    if ((!(dc = get_dc_ptr( hdc )))) return FALSE;

    if (size) *size = dc->attr->wnd_ext;

    if (dc->attr->map_mode == MM_ISOTROPIC || dc->attr->map_mode == MM_ANISOTROPIC)
    {
        if (!x_num || !x_denom || !y_num || !y_denom)
        {
            release_dc_ptr( dc );
            return FALSE;
        }

        dc->attr->wnd_ext.cx = (dc->attr->wnd_ext.cx * x_num) / x_denom;
        dc->attr->wnd_ext.cy = (dc->attr->wnd_ext.cy * y_num) / y_denom;
        if (dc->attr->wnd_ext.cx == 0) dc->attr->wnd_ext.cx = 1;
        if (dc->attr->wnd_ext.cy == 0) dc->attr->wnd_ext.cy = 1;
        if (dc->attr->map_mode == MM_ISOTROPIC) MAPPING_FixIsotropic( dc );
        DC_UpdateXforms( dc );
    }

    release_dc_ptr( dc );
    return TRUE;
}


/****************************************************************************
 *           ModifyWorldTransform   (GDI32.@)
 */
BOOL WINAPI ModifyWorldTransform( HDC hdc, const XFORM *xform, DWORD mode )
{
    BOOL ret = FALSE;
    DC *dc;

    if (!xform && mode != MWT_IDENTITY) return FALSE;
    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pModifyWorldTransform );
        if (dc->attr->graphics_mode == GM_ADVANCED)
            ret = physdev->funcs->pModifyWorldTransform( physdev, xform, mode );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetWorldTransform    (GDI32.@)
 */
BOOL WINAPI SetWorldTransform( HDC hdc, const XFORM *xform )
{
    BOOL ret = FALSE;
    DC *dc;

    if (!xform) return FALSE;
    /* The transform must conform to (eM11 * eM22 != eM12 * eM21) requirement */
    if (xform->eM11 * xform->eM22 == xform->eM12 * xform->eM21) return FALSE;

    TRACE("eM11 %f eM12 %f eM21 %f eM22 %f eDx %f eDy %f\n",
        xform->eM11, xform->eM12, xform->eM21, xform->eM22, xform->eDx, xform->eDy);

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetWorldTransform );
        if (dc->attr->graphics_mode == GM_ADVANCED)
            ret = physdev->funcs->pSetWorldTransform( physdev, xform );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           NtGdiSetVirtualResolution   (win32u.@)
 *
 * Undocumented on msdn.
 *
 * Changes the values of screen size in pixels and millimeters used by
 * the mapping mode functions.
 *
 * PARAMS
 *     hdc       [I] Device context
 *     horz_res  [I] Width in pixels  (equivalent to HORZRES device cap).
 *     vert_res  [I] Height in pixels (equivalent to VERTRES device cap).
 *     horz_size [I] Width in mm      (equivalent to HORZSIZE device cap).
 *     vert_size [I] Height in mm     (equivalent to VERTSIZE device cap).
 *
 * RETURNS
 *    TRUE if successful.
 *    FALSE if any (but not all) of the last four params are zero.
 *
 * NOTES
 *    This doesn't change the values returned by GetDeviceCaps, just the
 *    scaling of the mapping modes.
 *
 *    Calling with the last four params equal to zero sets the values
 *    back to their defaults obtained by calls to GetDeviceCaps.
 */
BOOL WINAPI NtGdiSetVirtualResolution( HDC hdc, DWORD horz_res, DWORD vert_res,
                                       DWORD horz_size, DWORD vert_size )
{
    DC * dc;
    TRACE("(%p %d %d %d %d)\n", hdc, horz_res, vert_res, horz_size, vert_size);

    if (!horz_res || !vert_res || !horz_size || !vert_size)
    {
        /* they must be all zero */
        if (horz_res || vert_res || horz_size || vert_size) return FALSE;
    }

    dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    dc->attr->virtual_res.cx  = horz_res;
    dc->attr->virtual_res.cy  = vert_res;
    dc->attr->virtual_size.cx = horz_size;
    dc->attr->virtual_size.cy = vert_size;

    release_dc_ptr( dc );
    return TRUE;
}

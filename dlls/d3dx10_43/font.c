/*
 * Copyright (C) 2008 Tony Wasserka
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
 *
 */

#define COBJMACROS
#include "d3dx10.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

#define D3DERR_INVALIDCALL 0x8876086c

struct d3dx_font
{
    ID3DX10Font ID3DX10Font_iface;
    LONG refcount;

    ID3D10Device *device;
};

static inline struct d3dx_font *impl_from_ID3DX10Font(ID3DX10Font *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx_font, ID3DX10Font_iface);
}

static HRESULT WINAPI d3dx_font_QueryInterface(ID3DX10Font *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DX10Font)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx_font_AddRef(ID3DX10Font *iface)
{
    struct d3dx_font *font = impl_from_ID3DX10Font(iface);
    ULONG refcount = InterlockedIncrement(&font->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);
    return refcount;
}

static ULONG WINAPI d3dx_font_Release(ID3DX10Font *iface)
{
    struct d3dx_font *font = impl_from_ID3DX10Font(iface);
    ULONG refcount = InterlockedDecrement(&font->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        ID3D10Device_Release(font->device);
        heap_free(font);
    }
    return refcount;
}

static HRESULT WINAPI d3dx_font_GetDevice(ID3DX10Font *iface, ID3D10Device **device)
{
    struct d3dx_font *font = impl_from_ID3DX10Font(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    if (!device) return D3DERR_INVALIDCALL;
    *device = font->device;
    ID3D10Device_AddRef(font->device);

    return S_OK;
}

static HRESULT WINAPI d3dx_font_GetDescA(ID3DX10Font *iface, D3DX10_FONT_DESCA *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx_font_GetDescW(ID3DX10Font *iface, D3DX10_FONT_DESCW *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);

    return E_NOTIMPL;
}

static BOOL WINAPI d3dx_font_GetTextMetricsA(ID3DX10Font *iface, TEXTMETRICA *metrics)
{
    FIXME("iface %p, metrics %p stub!\n", iface, metrics);

    return FALSE;
}

static BOOL WINAPI d3dx_font_GetTextMetricsW(ID3DX10Font *iface, TEXTMETRICW *metrics)
{
    FIXME("iface %p, metrics %p stub!\n", iface, metrics);

    return FALSE;
}

static HDC WINAPI d3dx_font_GetDC(ID3DX10Font *iface)
{
    FIXME("iface %p stub!\n", iface);

    return NULL;
}

static HRESULT WINAPI d3dx_font_GetGlyphData(ID3DX10Font *iface, UINT glyph,
        ID3D10ShaderResourceView **view, RECT *black_box, POINT *cell_inc)
{
    FIXME("iface %p, glyph %u, view %p, black_box %p, cell_inc %p stub!\n",
          iface, glyph, view, black_box, cell_inc);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx_font_PreloadCharacters(ID3DX10Font *iface, UINT first, UINT last)
{
    FIXME("iface %p, first %u, last %u stub!\n", iface, first, last);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx_font_PreloadGlyphs(ID3DX10Font *iface, UINT first, UINT last)
{
    FIXME("iface %p, first %u, last %u stub!\n", iface, first, last);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx_font_PreloadTextA(ID3DX10Font *iface, const char *string, INT count)
{
    WCHAR *wstr;
    HRESULT hr;
    int countW;

    TRACE("iface %p, string %s, count %d.\n", iface, debugstr_an(string, count), count);

    if (!string && !count)
        return S_OK;

    if (!string)
        return D3DERR_INVALIDCALL;

    countW = MultiByteToWideChar(CP_ACP, 0, string, count < 0 ? -1 : count, NULL, 0);

    if (!(wstr = heap_alloc(countW * sizeof(*wstr))))
        return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, string, count < 0 ? -1 : count, wstr, countW);

    hr = ID3DX10Font_PreloadTextW(iface, wstr, count < 0 ? countW - 1 : countW);

    heap_free(wstr);

    return hr;
}

static HRESULT WINAPI d3dx_font_PreloadTextW(ID3DX10Font *iface, const WCHAR *string, INT count)
{
    FIXME("iface %p, string %s, count %d stub!\n", iface, debugstr_wn(string, count), count);

    return E_NOTIMPL;
}

static INT WINAPI d3dx_font_DrawTextA(ID3DX10Font *iface, ID3DX10Sprite *sprite,
        const char *string, INT count, RECT *rect, UINT format, D3DXCOLOR color)
{
    int ret, countW;
    WCHAR *wstr;

    TRACE("iface %p, sprite %p, string %s, count %d, rect %s, format %#x, color {%.8e,%.8e,%.8e,%8e}.\n",
            iface, sprite, debugstr_an(string, count), count, wine_dbgstr_rect(rect), format,
            color.r, color.g, color.b, color.a);

    if (!string || !count)
        return 0;

    if (!(countW = MultiByteToWideChar(CP_ACP, 0, string, count < 0 ? -1 : count, NULL, 0)))
        return 0;

    if (!(wstr = heap_alloc_zero(countW * sizeof(*wstr))))
        return 0;

    MultiByteToWideChar(CP_ACP, 0, string, count < 0 ? -1 : count, wstr, countW);

    ret = ID3DX10Font_DrawTextW(iface, sprite, wstr, count < 0 ? countW - 1 : countW,
                              rect, format, color);

    heap_free(wstr);

    return ret;
}

static INT WINAPI d3dx_font_DrawTextW(ID3DX10Font *iface, ID3DX10Sprite *sprite,
        const WCHAR *string, INT count, RECT *rect, DWORD format, D3DXCOLOR color)
{
    FIXME("iface %p, sprite %p, string %s, count %d, rect %s, format %#x, color {%.8e,%.8e,%.8e,%.8e} stub!\n",
            iface, sprite, debugstr_wn(string, count), count, wine_dbgstr_rect(rect),
            format, color.r, color.g, color.b, color.a);

    return E_NOTIMPL;
}

static const ID3DX10FontVtbl d3dx_font_vtbl =
{
    /*** IUnknown methods ***/
    d3dx_font_QueryInterface,
    d3dx_font_AddRef,
    d3dx_font_Release,
    /*** ID3DX10Font methods ***/
    d3dx_font_GetDevice,
    d3dx_font_GetDescA,
    d3dx_font_GetDescW,
    d3dx_font_GetTextMetricsA,
    d3dx_font_GetTextMetricsW,
    d3dx_font_GetDC,
    d3dx_font_GetGlyphData,
    d3dx_font_PreloadCharacters,
    d3dx_font_PreloadGlyphs,
    d3dx_font_PreloadTextA,
    d3dx_font_PreloadTextW,
    d3dx_font_DrawTextA,
    d3dx_font_DrawTextW,
};

HRESULT WINAPI D3DX10CreateFontA(ID3D10Device *device, INT height, UINT width, UINT weight,
        UINT miplevels, BOOL italic, UINT charset, UINT precision, UINT quality,
        UINT pitchandfamily, const char *facename, ID3DX10Font **font)
{
    D3DX10_FONT_DESCA desc;

    TRACE("device %p, height %d, width %u, weight %u, miplevels %u, italic %#x, charset %u, "
            "precision %u, quality %u, pitchandfamily %u, facename %s, font %p.\n",
            device, height, width, weight, miplevels, italic, charset, precision, quality,
            pitchandfamily, debugstr_a(facename), font);

    if (!device || !font)
        return D3DERR_INVALIDCALL;

    desc.Height = height;
    desc.Width = width;
    desc.Weight = weight;
    desc.MipLevels = miplevels;
    desc.Italic = italic;
    desc.CharSet = charset;
    desc.OutputPrecision = precision;
    desc.Quality = quality;
    desc.PitchAndFamily = pitchandfamily;
    if (facename)
        lstrcpyA(desc.FaceName, facename);
    else
        desc.FaceName[0] = 0;

    return D3DX10CreateFontIndirectA(device, &desc, font);
}

HRESULT WINAPI D3DX10CreateFontW(ID3D10Device *device, INT height, UINT width, UINT weight,
        UINT miplevels, BOOL italic, UINT charset, UINT precision, UINT quality,
        UINT pitchandfamily, const WCHAR *facename, ID3DX10Font **font)
{
    D3DX10_FONT_DESCW desc;

    TRACE("device %p, height %d, width %u, weight %u, miplevels %u, italic %#x, charset %u, "
            "precision %u, quality %u, pitchandfamily %u, facename %s, font %p.\n",
            device, height, width, weight, miplevels, italic, charset, precision, quality,
            pitchandfamily, debugstr_w(facename), font);

    if (!device || !font)
        return D3DERR_INVALIDCALL;

    desc.Height = height;
    desc.Width = width;
    desc.Weight = weight;
    desc.MipLevels = miplevels;
    desc.Italic = italic;
    desc.CharSet = charset;
    desc.OutputPrecision = precision;
    desc.Quality = quality;
    desc.PitchAndFamily = pitchandfamily;
    if (facename)
        lstrcpyW(desc.FaceName, facename);
    else
        desc.FaceName[0] = '\0';

    return D3DX10CreateFontIndirectW(device, &desc, font);
}

HRESULT WINAPI D3DX10CreateFontIndirectA(ID3D10Device *device, const D3DX10_FONT_DESCA *desc,
        ID3DX10Font **font)
{
    D3DX10_FONT_DESCW descW;

    TRACE("device %p, desc %p, font %p.\n", device, desc, font);

    if (!device || !desc || !font)
        return D3DERR_INVALIDCALL;

    memcpy(&descW, desc, FIELD_OFFSET(D3DX10_FONT_DESCA, FaceName));
    MultiByteToWideChar(CP_ACP, 0, desc->FaceName, -1, descW.FaceName, ARRAY_SIZE(descW.FaceName));
    return D3DX10CreateFontIndirectW(device, &descW, font);
}

HRESULT WINAPI D3DX10CreateFontIndirectW(ID3D10Device *device, const D3DX10_FONT_DESCW *desc,
        ID3DX10Font **font)
{
    struct d3dx_font *object;

    TRACE("device %p, desc %p, font %p.\n", device, desc, font);

    if (!device || !desc || !font)
        return D3DERR_INVALIDCALL;

    if (!(object = heap_alloc_zero(sizeof(*object))))
    {
        *font = NULL;
        return E_OUTOFMEMORY;
    }

    object->ID3DX10Font_iface.lpVtbl = &d3dx_font_vtbl;
    object->refcount = 1;
    object->device = device;
    ID3D10Device_AddRef(device);

    *font = &object->ID3DX10Font_iface;

    return S_OK;
}

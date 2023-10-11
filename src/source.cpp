#include <VapourSynth.h>
#include <Windows.h>
#include <codecvt>
#include <d3d9.h>
#include <iostream>
#include <locale>
#include <vector>

typedef void(VS_CC *MADVRVapourSynthInit)(
    VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin, void *UpdateFrame, int version
);

struct MADVRDXFrameFormat {
        uint32_t fourcc;
        uint32_t field_4;
        uint32_t field_8;
        uint32_t field_C;
};

#define MADVR_LIBS_DIRPATH L"madVR\\"

#ifdef _WIN64
#define MADVRDLL_SUFFIX_LIB L"64.dll"
#define MADVRDLL_SUFFIX_AX L"64.ax"
#else
#define MADVRDLL_SUFFIX_LIB L"32.dll"
#define MADVRDLL_SUFFIX_AX L".ax"
#endif

const DWORD loadDwFlags = LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR;

std::wstring get_plugins_path() {
    HMODULE module;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR) &get_plugins_path, &module);

    std::vector<wchar_t> pathBuf(65536);
    GetModuleFileNameW(module, pathBuf.data(), (DWORD) pathBuf.size());

    std::wstring dllPath = pathBuf.data();
    dllPath.resize(dllPath.find_last_of('\\') + 1);

    return dllPath + MADVR_LIBS_DIRPATH;
}

HMODULE try_load_dll(
    const std::wstring plugins_path, const std::wstring lib_filename, const std::wstring platform_name,
    bool required = false, bool enable_global = true
) {
    HMODULE madvr_module = LoadLibraryExW((plugins_path + lib_filename).c_str(), nullptr, loadDwFlags);

    if (!madvr_module && enable_global)
        madvr_module = LoadLibraryExW(lib_filename.c_str(), nullptr, loadDwFlags);

    if (!madvr_module && required) {
        std::wcout << platform_name << L"-madVR: Failed to load " << plugins_path + lib_filename << std::endl;

        return nullptr;
    }

    return madvr_module;
}

template<typename T>
T get_platform_init(const std::wstring platform_name) {
    T madvr_init = nullptr;

    std::wstring plugins_path = get_plugins_path();

    try_load_dll(plugins_path, L"madHcNet" MADVRDLL_SUFFIX_LIB, platform_name);
    try_load_dll(plugins_path, L"mvrSettings" MADVRDLL_SUFFIX_LIB, platform_name);

    HMODULE madvr_ax = try_load_dll(plugins_path, L"madVR" MADVRDLL_SUFFIX_AX, platform_name, true);

    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    *(FARPROC *) &madvr_init = GetProcAddress(madvr_ax, converter.to_bytes(platform_name).c_str());

    if (!madvr_init)
        std::wcout << platform_name << "-madVR: Failed to find the " << platform_name << " init in madVR ax library"
                   << std::endl;

    return madvr_init;
}

template<typename T, bool single_texture, int c_offset, int bps, int bit_shift>
void upload_frame(
    D3DLOCKED_RECT *mlock, D3DLOCKED_RECT *clock, ptrdiff_t luma_stride, ptrdiff_t chroma_stride, const void *_src0_ptr,
    const void *_src1_ptr, const void *_src2_ptr, int luma_width, int luma_height, int chroma_width, int chroma_height
) {
    const T *src0_ptr = reinterpret_cast<const T *>(_src0_ptr);
    const T *src1_ptr = reinterpret_cast<const T *>(_src1_ptr);
    const T *src2_ptr = reinterpret_cast<const T *>(_src2_ptr);

    T *mdst_ptr = reinterpret_cast<T *>(mlock->pBits), *cdst_ptr;
    ptrdiff_t mdst_stride = mlock->Pitch / bps, cdst_stride;

    if constexpr (!single_texture) {
        cdst_ptr = reinterpret_cast<T *>(clock->pBits);
        cdst_stride = clock->Pitch / bps;
    }

    if constexpr (bps > 1) {
        luma_stride /= bps;
        chroma_stride /= bps;
    }

    for (int y = 0; y < luma_height; y++) {
        if constexpr (single_texture) {
            for (int x = 0; x < luma_width; x++) {
                mdst_ptr[x * 4 + 0] = src0_ptr[x] << bit_shift;
                mdst_ptr[x * 4 + 1] = src1_ptr[x] << bit_shift;
                mdst_ptr[x * 4 + 2] = src2_ptr[x] << bit_shift;
            }
        } else {
            for (int x = 0; x < luma_width; x++) {
                mdst_ptr[x] = src0_ptr[x] << bit_shift;

                if (x < chroma_width && y < chroma_height) {
                    cdst_ptr[x * 4 + 0 + c_offset] = src1_ptr[x] << bit_shift;
                    cdst_ptr[x * 4 + 1 + c_offset] = src2_ptr[x] << bit_shift;
                }
            }
        }

        mdst_ptr += mdst_stride;
        src0_ptr += luma_stride;

#define increment_chroma_stride \
    cdst_ptr += cdst_stride;    \
    src1_ptr += chroma_stride;  \
    src2_ptr += chroma_stride;

        if constexpr (single_texture) {
            increment_chroma_stride
        } else {
            if (y < chroma_height) {
                increment_chroma_stride
            }
        }

#undef increment_chroma_stride
    }
}

bool update_frame(
    int n, const uint8_t *plane0_read, const uint8_t *plane1_read, const uint8_t *plane2_read, int bit_depth,
    MADVRDXFrameFormat *frame_format, int luma_width, int luma_height, int luma_stride, int chroma_width,
    int chroma_height, int chroma_stride, IDirect3DTexture9 *luma_tex, IDirect3DTexture9 *chroma_tex,
    IDirect3DTexture9 *yuv444_tex, IDirect3DTexture9 *rgb_tex, bool *unused
) {
    bool single_texture = yuv444_tex || rgb_tex;

    IDirect3DTexture9 *main_tex = single_texture ? (yuv444_tex ? yuv444_tex : rgb_tex) : luma_tex;

    D3DLOCKED_RECT mlock, clock;

    if (FAILED(main_tex->LockRect(0, &mlock, NULL, 0))) {
        main_tex->UnlockRect(0);
        return false;
    }

    if (!single_texture && FAILED(chroma_tex->LockRect(0, &clock, NULL, 0))) {
        chroma_tex->UnlockRect(0);
        return false;
    }

    bool hbd = bit_depth > 8;

    const void *src0_ptr = single_texture ? (hbd ? plane0_read : plane2_read) : plane0_read;
    const void *src1_ptr = single_texture ? (hbd ? plane1_read : plane1_read) : (hbd ? plane1_read : plane2_read);
    const void *src2_ptr = single_texture ? (hbd ? plane2_read : plane0_read) : (hbd ? plane2_read : plane1_read);

#define args                                                                                                      \
    &mlock, &clock, (ptrdiff_t) luma_stride, (ptrdiff_t) chroma_stride, src0_ptr, src1_ptr, src2_ptr, luma_width, \
        luma_height, chroma_width, chroma_height

    switch (bit_depth) {
        case 16:
            single_texture ? upload_frame<uint16_t, true, 0, 2, 0>(args) : upload_frame<uint16_t, false, 0, 2, 0>(args);
            break;
        case 12:
            single_texture ? upload_frame<uint16_t, true, 0, 2, 4>(args) : upload_frame<uint16_t, false, 0, 2, 4>(args);
            break;
        case 10:
            single_texture ? upload_frame<uint16_t, true, 0, 2, 6>(args) : upload_frame<uint16_t, false, 0, 2, 6>(args);
            break;
        case 8:
            single_texture ? upload_frame<uint8_t, true, 1, 1, 0>(args) : upload_frame<uint8_t, false, 1, 1, 0>(args);
            break;
    }

#undef args

    main_tex->UnlockRect(0);
    if (!single_texture)
        chroma_tex->UnlockRect(0);

#ifdef MADVR_DEBUG
    uint32_t fcc = frame_format->fourcc;

    if (rgb_tex) {
        printf("\nFrame format is RGB");
    } else {
        printf("\nFrame format is %c%c%c%c", fcc & 0xff, fcc >> 8 & 0xff, fcc >> 16 & 0xff, fcc >> 24 & 0xff);
    }

    printf("\nfield4: %d, field8: %d, fieldC: %d", frame_format->field_4, frame_format->field_8, frame_format->field_C);

    printf(
        "\nluma_tex: %d, chroma_tex: %d, yuv444_tex: %d, rgb_tex: %d", !!luma_tex, !!chroma_tex, !!yuv444_tex, !!rgb_tex
    );

    auto printTexFmtInfo = [](IDirect3DTexture9 *tex, const char *name) {
        if (tex) {
            D3DSURFACE_DESC desc;
            if (FAILED(tex->GetLevelDesc(0, &desc))) {
                printf("\nFailed getting level desc for %s\n", name);
                return;
            }

            printf("\nFormat for %s: %d, %d, %d, %d\n", name, desc.Format, desc.Width, desc.Height, desc.Type);
        }
    };

    printTexFmtInfo(main_tex, "main_tex");

    printTexFmtInfo(luma_tex, "luma_tex");
    printTexFmtInfo(chroma_tex, "chroma_tex");
    printTexFmtInfo(yuv444_tex, "yuv444_tex");
    printTexFmtInfo(rgb_tex, "rgb_tex");
#endif

    return true;
}

VS_EXTERNAL_API(void)
VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin) {
    auto madvr_vs_init = get_platform_init<MADVRVapourSynthInit>(L"VapourSynth");

    if (!madvr_vs_init) {
        configFunc("madshi.madVR", "madVR", "madVR Toolbox", VAPOURSYNTH_API_VERSION, true, plugin);

        return;
    }

    madvr_vs_init(configFunc, registerFunc, plugin, (void *) update_frame, 1);
}

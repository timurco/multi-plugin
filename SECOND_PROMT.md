Отлично, мне очень все нравится. Но есть некоторые моменты:
    
1. GlobalData у After effects конечно есть свой - in_data->globalData, но это не совсем универсальное решение и в некоторых моментах ограниченное, поэтому я обычно использую Global Singleton Class, который активируется в первый раз при запуске плагина (в АЕ в GlobalSetup, в OFX при создании класса)
    
2. Надо понимать что порядок байт в АЕ при PixelFormat нестандартный: ARGB, а также есть 8бит, 16бит (uint16_t, у которого максимальное число странное - 32768, хотя у других обычно это 65535), а также 32бита (float). В Premiere похожая тема но аналоги у 16бит и 32бита имеют другие значения, например PF_PixelFormat_ARGB32 (АЕ 8 бит) == PrPixelFormat_ARGB_4444_8u (Premiere 8 bit) смотри файлы: /Users/timurko/Code/BSKL_Dependencies/adobe-ae-sdk-2025/Examples/Headers/PrSDKPixelFormat.h и /Users/timurko/Code/BSKL_Dependencies/adobe-ae-sdk-2025/Examples/Headers/AE_EffectPixelFormat.h. Давай чтобы не усложнять использовать только PF_PixelFormat_ARGB32, PF_PixelFormat_ARGB64, PF_PixelFormat_ARGB128, PF_PixelFormat_ARGB64, PF_PixelFormat_ARGB128. А PrPixelFormat_ARGB_4444_8u аналогичен по значению с PF_PixelFormat_ARGB32, поэтому его можно пропустить.
    
3. У тебя есть getInput, но он не учитывает, что в АЕ можно брать кадры по определенному времени, в АЕ это можно делать через CHECKOUT:
    
```cpp
    // Checkout frames using PF_CHECKOUT_PARAM like in Adobe SDK
    for (unsigned int i = 0; i < uniforms.num_frames; i++) {
        AEFX_CLR_STRUCT(checkout_frames[i]);
        
        // Calculate time offset for previous frames (i=0 is current, i=1 is previous, etc.)
        A_long time_offset = in_data->current_time - (i * in_data->time_step);
        
        ERR(PF_CHECKOUT_PARAM(in_data,
                              PLUGIN_INPUT, // Use input layer parameter
                              time_offset,
                              in_data->time_step,
                              in_data->time_scale,
                              &checkout_frames[i]));
        
        if (!err && checkout_frames[i].u.ld.data) {
            input_worlds[i] = MakeSimpleWorld(&checkout_frames[i].u.ld);
        } else {
            LOG_ERR << "Frame " << i << " is not available";
            err = PF_Err_BAD_CALLBACK_PARAM;
        }
    }
```

    И в ofx через fetchImage:
```cpp
    for (int i = lastFrame; i >= 0; i--) {
        // Set the parameters
        imageProcessor.setParameters(KernelParams{
            .percent = 1.0f - (float)i / lastFrame,
        });

        // fetch main input image of current frame
        double                      time = args.time - i * framesOffset;
        std::unique_ptr<OFX::Image> cur(m_SrcClip->fetchImage(time));

        if (cur.get()) {
            OFX::BitDepthEnum       srcBitDepth = cur->getPixelDepth();
            OFX::PixelComponentEnum srcComponents = cur->getPixelComponents();

            // see if they have the same depths and bytes and all
            if (srcBitDepth != dstBitDepth || srcComponents != dstComponents) OFX::throwSuiteStatusException(kOfxStatErrValue);
        } else {
            continue;
        }
        
        auto curBounds = dst->getBounds();
        auto curRowBytes = dst->getRowBytes();
        auto curComp = cur->getPixelComponentCount();
        auto curBitDepthInfo = getDepthInfo(dst->getPixelDepth());
        auto curCompString = cur->getPixelComponentsProperty();
        //....
    }
```

4. Metal/OpenCL/CUDA это конечно хорошо, но я в последнее время делаю плагины на WebGPU которые просто копирую из CPU кадра туда-сюда. Но я предлагаю пока просто сделать MultiThread рендеринг

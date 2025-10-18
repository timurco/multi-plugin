




### 1. Про Stubs и PF_ValueDisplayFlag_PERCENT

**В оригинале UParams:**
```cpp
// В OFX тоже используются AE флаги!
mp::floatSlider(0, &Params::mix, "Mix", 0, 100, 100.0f, 
                PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT)
// ↑ Работает и в AE, и в OFX благодаря stubs
```

**В нашей архитектуре с fluent API:**
```cpp
mp::floatSlider(0, &Params::mix, "Mix", 0, 100, 100.0f)
    .percent()  // ← Вместо флага
    .precision(2)
```

**Решение:**
- ✅ **Оставим stubs для обратной совместимости** (если кто-то хочет старый стиль)
- ✅ **Добавим fluent API методы** (.percent(), .precision()) для удобства
- ✅ **Оба стиля работают:**
  ```cpp
  // Старый стиль (с флагами) - работает в AE и OFX
  mp::floatSlider(0, &Params::mix, "Mix", 0, 100, 100.0f, 
                  PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT)
  
  // Новый стиль (fluent) - тоже работает везде
  mp::floatSlider(0, &Params::mix, "Mix", 0, 100, 100.0f)
      .percent()
      .precision(2)
  ```

### 2. Обязательные features - точная спецификация:

#### Feature 1: Button onClick с callback
```cpp
mp::button(BUTTON_ID, "Randomize", "Generate random seed")
    .onClick([](auto& ctx) {
        ctx.setValue(&MyParams::seed, generateRandom());
        ctx.setValue(&MyParams::mix, 50.0f);
    })
```

**Как это работает:**
- `ctx` - это `ParamContext<MyParams>`
- `ctx.setValue()` - изменяет параметр через Source (AE или OFX)
- Lambda вызывается в `handleParameterChange()` когда нажата кнопка

#### Feature 2: Fluent API для всех параметров
```cpp
// Float slider
mp::floatSlider(0, &MyParams::mix, "Mix", 0, 100, 100.0f)
    .percent()
    .precision(2)
    .sliderRange(0, 200)  // Независимый от value range

// Integer slider
mp::intSlider(1, &MyParams::count, "Count", 1, 1000, 100)
    .sliderRange(1, 100)  // UI slider 1-100, но value может быть до 1000

// Checkbox (fluent не особо нужен, но для единообразия)
mp::checkbox(2, &MyParams::enabled, "Enabled", true)

// Color
mp::color(3, &MyParams::tint, "Tint", {1.0f, 0.8f, 0.3f, 1.0f})

// Popup
mp::popup(4, &MyParams::mode, "Mode", "Fast|Quality|Extreme", 0)

// Button с callback
mp::button(5, "Reset", "Reset all parameters")
    .onClick([](auto& ctx) {
        ctx.setValue(&MyParams::mix, 100.0f);
        ctx.setValue(&MyParams::enabled, true);
    })
```

### 3. Архитектура реализации:

```
include/multiplugin/params/
├── specs.hpp          - SpecFloat, SpecInt, SpecButton, etc.
├── handles.hpp        - HFloat, HInt, etc. для AE и OFX
├── param.hpp          - Param<Bag, Spec, Handle, Val>
├── param_builder.hpp  - FloatSliderBuilder, ButtonBuilder с fluent методами
├── sources.hpp        - AESource, OfxSource (fetch/set параметров)
├── builders.hpp       - AEBuilder, OfxBuilder (создание параметров)
├── param_set.hpp      - ParamSet<Bag, Ps...>
├── helpers.hpp        - floatSlider(), button(), etc. - возвращают Builders
├── context.hpp        - ParamContext для callbacks
└── params.hpp         - Main header
```

### 4. Ключевые классы:

#### ParamBuilder с fluent API:
```cpp
template<class Bag>
class FloatSliderBuilder {
    Param<Bag, SpecFloat, HFloat, float> param_;
    
public:
    FloatSliderBuilder(unsigned int disk_id, float Bag::*member, 
                      const char* name, float min, float max, float def);
    
    FloatSliderBuilder& percent() {
        param_.spec.display_flags |= PF_ValueDisplayFlag_PERCENT;
        return *this;
    }
    
    FloatSliderBuilder& precision(int prec) {
        param_.spec.precision = prec;
        return *this;
    }
    
    FloatSliderBuilder& sliderRange(float smin, float smax) {
        param_.spec.slider_min = smin;
        param_.spec.slider_max = smax;
        return *this;
    }
    
    // Неявное преобразование в Param для makeSet
    operator Param<Bag, SpecFloat, HFloat, float>() const { return param_; }
};
```

#### ButtonBuilder с onClick:
```cpp
template<class Bag>
class ButtonBuilder {
    Param<Bag, SpecButton, HButton, std::nullptr_t> param_;
    std::function<void(ParamContext<Bag>&)> callback_;
    
public:
    ButtonBuilder(unsigned int disk_id, const char* name, const char* label);
    
    ButtonBuilder& onClick(std::function<void(ParamContext<Bag>&)> cb) {
        callback_ = cb;
        return *this;
    }
    
    void executeCallback(ParamContext<Bag>& ctx) const {
        if (callback_) callback_(ctx);
    }
    
    operator Param<Bag, SpecButton, HButton, std::nullptr_t>() const { 
        return param_; 
    }
};
```

#### ParamContext для callbacks:
```cpp
template<typename Bag>
class ParamContext {
    Bag& bag_;
    const void* source_;  // AESource* или OfxSource*
    const ParamSet<Bag, ...>* paramSet_;
    
public:
    // Получить значение
    template<typename T>
    T getValue(T Bag::*member) const { return bag_.*member; }
    
    // Установить значение (вызывает paramSet_->setValue через source)
    template<typename T>
    void setValue(T Bag::*member, const T& value) {
        bag_.*member = value;
        paramSet_->setValue(*source_, member, value);
    }
    
    // Доступ к Bag
    Bag& getBag() { return bag_; }
};
```

### 5. Helper functions возвращают Builders:

```cpp
namespace mp {

// Float slider возвращает builder
template<class Bag>
auto floatSlider(unsigned int disk_id, float Bag::*member, 
                const char* name, float min, float max, float def) {
    return FloatSliderBuilder<Bag>(disk_id, member, name, min, max, def);
}

// Button возвращает builder
template<class Bag = void>
auto button(unsigned int disk_id, const char* name, const char* label) {
    return ButtonBuilder<Bag>(disk_id, name, label);
}

// Checkbox тоже builder (хоть fluent методов и нет)
template<class Bag>
auto checkbox(unsigned int disk_id, bool Bag::*member, 
             const char* name, bool def = false) {
    return CheckboxBuilder<Bag>(disk_id, member, name, def);
}

} // namespace mp
```

### 6. Использование:

```cpp
struct MyParams {
    float mix = 100.0f;
    float color[4] = {1.0f, 0.8f, 0.3f, 1.0f};
    int seed = 42;
    bool enabled = true;
    int mode = 0;
};

inline static auto kParams = mp::makeSet<MyParams>(
    // Fluent API с методами
    mp::floatSlider(0, &MyParams::mix, "Mix", 0, 100, 100.0f)
        .percent()
        .precision(2),
    
    mp::color(1, &MyParams::color, "Color", {1.0f, 0.8f, 0.3f, 1.0f}),
    
    mp::intSlider(2, &MyParams::seed, "Seed", 1, 1000, 42),
    
    mp::checkbox(3, &MyParams::enabled, "Enabled", true),
    
    mp::popup(4, &MyParams::mode, "Mode", "Fast|Quality|Extreme", 0),
    
    // Button с callback
    mp::button(5, "Randomize", "Generate random values")
        .onClick([](auto& ctx) {
            ctx.setValue(&MyParams::seed, rand() % 1000);
            ctx.setValue(&MyParams::mix, 50.0f + (rand() % 50));
        }),
    
    mp::button(6, "Reset", "Reset to defaults")
        .onClick([](auto& ctx) {
            ctx.setValue(&MyParams::mix, 100.0f);
            ctx.setValue(&MyParams::enabled, true);
        })
);
```

### 7. Интеграция в handleParameterChange:

```cpp
void handleParameterChange(int disk_id, const Source& source) {
    MyParams current;
    kParams.fetch(source, current);
    
    if (disk_id == 5) {  // Randomize button
        ParamContext<MyParams> ctx(current, &source, &kParams);
        kParams.executeButtonCallback(disk_id, ctx);
    }
    else if (disk_id == 6) {  // Reset button
        ParamContext<MyParams> ctx(current, &source, &kParams);
        kParams.executeButtonCallback(disk_id, ctx);
    }
}
```

### 8. Про stubs - финальное решение:

**Оставляем stubs с typedef для PF_ типов в OFX:**
```cpp
#ifdef BUILD_FOR_OFX
// Stubs чтобы использовать AE флаги в OFX
using PF_ValueDisplayFlags = int;
constexpr PF_ValueDisplayFlags PF_ValueDisplayFlag_NONE = 0;
constexpr PF_ValueDisplayFlags PF_ValueDisplayFlag_PERCENT = 1;

enum PF_Precision {
    PF_Precision_INTEGER = 0,
    PF_Precision_TENTHS = 1,
    PF_Precision_HUNDREDTHS = 2,
    PF_Precision_THOUSANDTHS = 3,
    PF_Precision_TEN_THOUSANDTHS = 4
};
#endif
```

**Тогда работают ОБА стиля:**
```cpp
// Стиль 1: С флагами (UParams-style)
mp::floatSlider(0, &Params::mix, "Mix", 0, 100, 100.0f, 
                PF_Precision_HUNDREDTHS, PF_ValueDisplayFlag_PERCENT)

// Стиль 2: Fluent API (новый)
mp::floatSlider(0, &Params::mix, "Mix", 0, 100, 100.0f)
    .percent()
    .precision(2)
    
    
    
-----



Отлично, понял!

## Уточненный план:

### 1. Изучить структуру оригинального UParams.hpp
- Посмотреть как организованы namespace и классы
- Понять порядок определения типов (specs, handles, builders, sources)
- Изучить API функций (floatSlider, checkbox, etc.)

### 2. Скопировать структуру в MultiPlugin с разбивкой на файлы
**Сохраним модульную структуру для удобства:**
```
include/multiplugin/params/
├── specs.hpp          - Спецификации параметров (SpecFloat, SpecInt, etc.)
├── handles.hpp        - Host-specific handles (HFloat, HInt, etc.)
├── param.hpp          - Template Param<Bag, Spec, Handle, Val>
├── sources.hpp        - AESource и OfxSource
├── builders.hpp       - AEBuilder и OfxBuilder
├── param_set.hpp      - ParamSet<Bag, Ps...>
├── helpers.hpp        - Helper functions (floatSlider, checkbox, etc.)
└── params.hpp         - Main header, включает всё
```

**Преимущества модульности:**
- ✅ Легко добавить новый тип параметра (добавить Spec в specs.hpp, handle в handles.hpp, helper в helpers.hpp)
- ✅ Легко расширить builder новыми методами
- ✅ Можно включить только нужные части
- ✅ Понятная структура для навигации

### 3. Про "AE SDK stubs"
В оригинальном UParams.hpp есть такой код:
```cpp
#ifdef BUILD_FOR_AE
#   include <AE_Effect.h>
    // ... реальные AE типы
#else
// ── AE stubs, used when compiling only OFX (or for IntelliSense) ──
using  PF_ParamUIFlags        = int;
using  PF_ValueDisplayFlags   = int;
constexpr PF_ValueDisplayFlags PF_ValueDisplayFlag_PERCENT = 1;
// ... fake типы для IDE
#endif
```

**Зачем это в UParams:** Чтобы OFX плагин мог компилироваться без AE SDK, и IDE показывала автодополнение.

**Почему нам это НЕ нужно:**
- У нас есть переменные окружения AE_SDK_PATH и OFX_PATH
- Мы не компилируем оба плагина одновременно
- CMake контролирует BUILD_FOR_AE и BUILD_FOR_OFX
- Не нужны fake типы - есть реальные SDK

**Что сделаем:** Просто используем реальные типы из SDK, без fallback'ов.

### 4. Ключевые адаптации:

#### API функций (точно как в UParams):
```cpp
// Float slider - классический UParams стиль
template<class Bag>
auto floatSlider(unsigned int disk_id, float Bag::*m, const char* ui,
                 float vmin, float vmax, float def,
                 int prec = PF_Precision_TENTHS, 
                 PF_ValueDisplayFlags d = PF_ValueDisplayFlag_NONE);

// Checkbox
template<class Bag>
auto checkbox(unsigned int disk_id, bool Bag::*m, const char* ui, bool def = false);

// Button
template<class Bag = void>
auto button(unsigned int disk_id, const char* ui, const char* label);

// И так далее для всех типов...
```

#### Использование (точно как в UParams):
```cpp
struct MyParams {
    float mix = 100.0f;
    int seed = 42;
    bool enabled = true;
};

inline static auto kParams = mp::makeSet<MyParams>(
    mp::floatSlider(0, &MyParams::mix, "Mix", 
                    0, 100, 100.0f, 
                    PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT),
    mp::intSlider(1, &MyParams::seed, "Seed", 1, 1000, 42),
    mp::checkbox(2, &MyParams::enabled, "Enabled", true),
    mp::button(3, "Reset", "Reset to defaults")
);
```

### 5. Масштабируемость - как легко добавлять параметры:

**Сценарий:** Хочешь добавить новый тип "Gradient" параметра

**Шаг 1:** Добавить spec в `specs.hpp`:
```cpp
struct SpecGradient {
    const char* name;
    // ... gradient properties
    unsigned int disk_id;
    std::string unique_name;
};
```

**Шаг 2:** Добавить handles в `handles.hpp`:
```cpp
#ifdef BUILD_FOR_AE
struct HGradient { int id; };
#endif
#ifdef BUILD_FOR_OFX
struct HGradient { void* param; };
#endif
```

**Шаг 3:** Добавить helper в `helpers.hpp`:
```cpp
template<class Bag>
auto gradient(unsigned int disk_id, GradientData Bag::*m, 
              const char* name, /* ... */) {
    return Param<Bag, SpecGradient, HGradient, GradientData>{...};
}
```

**Шаг 4:** Добавить методы в builders (builders.hpp):
```cpp
// В AEBuilder:
HGradient gradient(unsigned int disk_id, const std::string& unique_name, ...);

// В OfxBuilder:
HGradient gradient(unsigned int disk_id, const std::string& unique_name, ...);
```

**Шаг 5:** Добавить fetch/setValue logic в `param_set.hpp` implementations

**Готово!** Теперь можно использовать:
```cpp
mp::gradient(10, &Params::myGradient, "Gradient", ...)
```

### 6. Интеграция с MultiPlugin:

**В backends:**
```cpp
// ae_backend.cpp - ParamsSetup
mp::AEBuilder builder(in_data, out_data);
g_plugin->getParams().build(builder);

// ae_backend.cpp - UserChangedParam
mp::AESource source(in_data, out_data, params);
int disk_id = g_plugin->getParams().getDiskIdByParamIndex(extra->param_index);
// Handle parameter change...

// ofx_backend.cpp - describeInContext
mp::OfxBuilder builder(desc);
g_plugin->getParams().build(builder);

// ofx_backend.cpp - instanceChanged
mp::OfxSource source(*this);
int disk_id = g_plugin->getParams().getDiskIdByUniqueName(param_name);
// Handle parameter change...
```

**В RenderContext:**
```cpp
// Добавить метод для получения параметров
template<typename ParamBag>
ParamBag getParams(const ParamSet<ParamBag>& paramSet) {
    ParamBag bag;
    // Fetch from source
    return bag;
}
```

Всё верно понял?
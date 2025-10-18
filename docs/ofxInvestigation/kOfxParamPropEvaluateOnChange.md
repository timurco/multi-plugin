## Когда вызывается `changedParam` (`kOfxActionInstanceChanged`)?

**`changedParam` вызывается НЕ ВСЕГДА!** Есть специальный флаг, который контролирует это поведение.

### Ключевой флаг: `kOfxParamPropEvaluateOnChange`

Из документации `ofxParam.h:334-343`:

```cpp
/** @brief Flags whether changing a parameter's value forces an evaluation (ie: render),
    - Type - int x 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read/write)
    - Default - 1  ⬅️ По умолчанию ВКЛЮЧЕНО!
    - Valid Values - 0 or 1

This is used to indicate if the value of a parameter has any affect on an 
effect's output, eg: the parameter may be purely for GUI purposes, and so 
changing its value should not trigger a re-render.
*/
#define kOfxParamPropEvaluateOnChange "OfxParamPropEvaluateOnChange"
```

**Что это означает:**

| Значение | Поведение |
|----------|-----------|
| **1** (default) | Изменение параметра → вызывается `changedParam` → триггерится рендер |
| **0** | Изменение параметра → **НЕ** вызывается `changedParam` → **НЕ** триггерится рендер |

## Пример использования

### Параметр, который НЕ влияет на рендер:

```cpp
// В describe:
BooleanParamDescriptor *debugMode = desc.defineBooleanParam("showDebugInfo");
debugMode->setLabel("Show Debug Info");
debugMode->setHint("Displays debug information in the UI");
debugMode->setDefault(false);
debugMode->setEvaluateOnChange(false);  // ⬅️ НЕ вызывать changedParam!
//                                          Это чисто UI параметр
```

**Результат:**
- Когда пользователь переключает `showDebugInfo`, `changedParam` **НЕ** вызывается
- Рендер **НЕ** перезапускается
- Этот параметр можно использовать только в UI overlay/interact

### Параметр, который ВЛИЯЕТ на рендер (по умолчанию):

```cpp
// В describe:
DoubleParamDescriptor *blur = desc.defineDoubleParam("blurSize");
blur->setLabel("Blur Size");
blur->setDefault(0.0);
blur->setRange(0, 100);
// setEvaluateOnChange по умолчанию = true, можно не указывать
blur->setEvaluateOnChange(true);  // Можно явно указать для ясности
```

**Результат:**
- Когда пользователь меняет `blurSize`, `changedParam` **ВЫЗЫВАЕТСЯ**
- Триггерится рендер
- Параметр влияет на выходное изображение

## Причины использования `kOfxChangeReason`

Когда `changedParam` вызывается, хост передаёт **причину** изменения через property `kOfxPropChangeReason`:

```cpp
// Из ofxCore.h:756-764
#define kOfxPropChangeReason "OfxPropChangeReason"

// Возможные значения:
#define kOfxChangeUserEdited   "OfxChangeUserEdited"    // Пользователь изменил
#define kOfxChangePluginEdited "OfxChangePluginEdited"  // Плагин сам изменил
#define kOfxChangeTime         "OfxChangeTime"          // Изменилось время
```

### Пример обработки в Support Library:

```cpp
// В basic.cpp:441
void BasicPlugin::changedParam(
    const OFX::InstanceChangedArgs &args, 
    const std::string &paramName)
{
    // args содержит:
    // - args.reason: причина изменения (user/plugin/time)
    // - args.time: время изменения
    // - args.renderScale: текущий render scale
    
    if (paramName == "scaleComponents") {
        setEnabledness();  // Обновляем enabled состояние других параметров
    }
}
```

## Полная структура `InstanceChangedArgs`:

```cpp
// Из Support Library
class InstanceChangedArgs {
public:
    double time;                  // Время изменения
    OfxPointD renderScale;        // Render scale
    ChangeReasonEnum reason;      // Причина изменения
    
    // reason может быть:
    // - eChangeUserEdit   - пользователь изменил в UI
    // - eChangePluginEdit - плагин программно изменил
    // - eChangeTime       - время изменилось (для анимированных параметров)
};
```

## Практические примеры:

### 1. Параметр для UI overlay (не влияет на рендер):

```cpp
// Показывать ли сетку в overlay
BooleanParamDescriptor *showGrid = desc.defineBooleanParam("showGrid");
showGrid->setLabel("Show Grid");
showGrid->setEvaluateOnChange(false);  // ❌ НЕ вызывать changedParam
showGrid->setAnimates(false);          // Не анимируется
```

### 2. Параметр для анализа (триггерит changedParam, но не рендер):

```cpp
// Кнопка для анализа изображения
PushButtonParamDescriptor *analyze = desc.definePushButtonParam("analyzeBtn");
analyze->setLabel("Analyze Image");
analyze->setEvaluateOnChange(true);   // ✅ Вызывать changedParam
// Но в changedParam мы не меняем изображение, а только анализируем

void MyPlugin::changedParam(const InstanceChangedArgs &args, 
                            const std::string &paramName) {
    if (paramName == "analyzeBtn") {
        // Анализируем текущий кадр
        analyzeCurrentFrame();
        // Обновляем другие параметры с результатами
        resultParam_->setValue(analysisResult);
    }
}
```

### 3. Зависимые параметры:

```cpp
// В describe:
BooleanParamDescriptor *useCustomSize = 
    desc.defineBooleanParam("useCustomSize");
useCustomSize->setLabel("Use Custom Size");
useCustomSize->setDefault(false);
useCustomSize->setEvaluateOnChange(true);  // Влияет на UI и рендер

DoubleParamDescriptor *customSize = desc.defineDoubleParam("customSize");
customSize->setLabel("Custom Size");
customSize->setDefault(100);
customSize->setEnabled(false);  // Изначально disabled
customSize->setEvaluateOnChange(true);

// В changedParam:
void MyPlugin::changedParam(const InstanceChangedArgs &args, 
                            const std::string &paramName) {
    if (paramName == "useCustomSize") {
        bool useCustom = useCustomSize_->getValueAtTime(args.time);
        customSize_->setEnabled(useCustom);  // Включаем/выключаем
    }
}
```

## Важные детали:

### 1. `changedParam` вызывается при изменении через:

✅ Пользователя (UI)
✅ Плагин (programmatically: `setValue()`, `setValueAtTime()`)
✅ Undo/Redo операции
✅ Загрузка проекта (с восстановлением значений)
✅ Keyframe изменения (если анимирован)

### 2. `changedParam` НЕ вызывается при:

❌ `setEvaluateOnChange(false)` установлен
❌ Параметр secret и не persistent
❌ В action `kOfxActionDescribe` (нет инстанса)

### 3. Рендер триггерится только если:

- `setEvaluateOnChange(true)` (default)
- И параметр не secret или изменён программно

## Специальный случай: `changedClip`

Есть также отдельный callback для изменения **клипов** (входов):

```cpp
// В basic.cpp:448
void BasicPlugin::changedClip(
    const OFX::InstanceChangedArgs &args, 
    const std::string &clipName)
{
    if (clipName == kOfxImageEffectSimpleSourceClipName) {
        setEnabledness();  // Обновляем UI в зависимости от типа клипа
    }
}
```

## Резюме:

```cpp
┌─────────────────────────────────────────────────────────────┐
│  Пользователь меняет параметр в UI                          │
└────────────────┬────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────┐
│  Хост проверяет: kOfxParamPropEvaluateOnChange == 1?        │
└────────┬─────────────────────────────┬──────────────────────┘
         │ ДА (default)                │ НЕТ
         ▼                             ▼
┌────────────────────┐     ┌───────────────────────────┐
│ Вызывает:          │     │ НИЧЕГО не делает          │
│ - changedParam()   │     │ - changedParam НЕ вызван  │
│ - Триггерит рендер │     │ - Рендер НЕ триггерится   │
└────────────────────┘     └───────────────────────────┘
```

**Золотое правило:**
> По умолчанию `changedParam` вызывается при любом изменении параметра. Чтобы **отключить** это, нужно явно вызвать `setEvaluateOnChange(false)`.

Используйте `setEvaluateOnChange(false)` для:
- UI-only параметров (показать/скрыть оверлей)
- Debug опций
- Параметров, которые не влияют на пиксели
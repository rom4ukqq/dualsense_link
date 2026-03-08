# Быстрый старт (Windows 10/11)

## 1) Проверка окружения

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\preflight.ps1
```

Скрипт проверяет:
- `CMake`
- `MSBuild`
- наличие `WDK`
- наличие toolchain для `WinUI 3`
- режим `test-signing`
- права администратора (если требуются для выбранной операции)

## 2) Сборка backend (CMake)

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_backend.ps1 -Config Debug
```

## 3) Сборка WinUI 3

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_ui.ps1 -Config Debug
```

Если пакеты уже восстановлены локально:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_ui.ps1 -Config Debug -SkipRestore
```

## 4) Запуск приложения

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_app.ps1 -Config Debug
```

Либо короткий wrapper:

```powershell
.\run_ui.ps1
```

## 5) Установка драйвера (test-signing)

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_driver.ps1
```

Если test-signing отключен:

```powershell
bcdedit /set testsigning on
```

Перезагрузить Windows и повторить установку.

## Что должно работать на этапе 1

- Ввод DualSense по Bluetooth.
- Перевод BT input -> USB input и отправка в virtual HID.
- Обратный путь output report (игра -> драйвер -> сервис -> Bluetooth-контроллер).
- Статус в WinUI: подключение, батарея, bridge mode, raw stream, HidHide.

Аудио через контроллер в этап 1 не входит.

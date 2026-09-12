# CarShowCase

Interactive 3D vehicle showcase built with Unreal Engine 5.7. The project focuses on real-time product visualization, allowing users to inspect different vehicles through predefined camera views, an orbital view, runtime material customization, and an interactive UI.

## Overview

CarShowCase was developed as an interactive showroom rather than a traditional game. The project combines Unreal Engine's real-time 3D environment with custom C++ logic and UMG/Slate UI to present multiple vehicle models in a configurable showroom.

The main runtime system is `APremiumGarageController`, which coordinates vehicle selection, camera states, material changes, showroom state, UI updates, and vehicle transitions.

## Project Focus

- Unreal Engine 5.7 development
- C++ gameplay/runtime programming
- Interactive 3D product visualization
- Real-time camera and scene control
- Runtime material customization
- UMG/Slate interface development
- Unreal Variant Manager integration

## Features

### Vehicle Selection

The showroom currently exposes five vehicle entries through the runtime UI:

- Lamborghini Murcielago 2001
- Lamborghini Urus 2018
- Nissan Fairlady 300ZX Z32 1989
- Toyota AE86 Sprinter Trueno Zenki
- High-Poly Porsche Singer

Only the selected vehicle is made visible while the controller updates the active vehicle state.

### Inspection Views

The C++ controller supports predefined inspection views for:

- Front
- Wheels
- Rear / spoiler
- Motor
- Orbital view

The motor view selects a different camera depending on the active vehicle. The orbital mode continuously rotates the camera around the selected vehicle.

### Runtime Material Customization

Vehicle paint can be modified at runtime using dynamic material instances. The controller identifies paint material slots for each vehicle, applies the selected color, and keeps the original materials available for restoration.

The UI also provides predefined paint colors and RGB sliders.

### Showroom Customization

The showroom wall/background color can be changed at runtime through predefined color states or a custom color value.

### Runtime UI

The project contains a custom `UPremiumGarageWidget` derived from `UUserWidget`. The widget builds and manages controls for:

- Vehicle selection
- Inspection views
- Paint colors
- RGB color adjustment
- Showroom colors
- Vehicle information

## Technical Implementation

### Premium Garage Controller

`APremiumGarageController` is the central runtime controller. It maintains the active vehicle and inspection state and exposes Blueprint-callable operations for the main presentation features.

Responsibilities include:

- Finding vehicles and cameras through Unreal Actor Tags
- Managing vehicle visibility
- Switching inspection cameras
- Updating the orbital camera
- Applying and resetting vehicle materials
- Caching original paint materials
- Applying showroom wall colors
- Driving vehicle selection transitions
- Updating the runtime UI

The controller runs through `Tick()` to update orbital camera motion and active vehicle transitions.

### Runtime UI

`UPremiumGarageWidget` extends `UUserWidget` and creates the presentation controls using UMG/Slate components such as buttons, combo boxes, sliders, text blocks, and layout containers.

The widget communicates with `APremiumGarageController` through direct controller references and event handlers for the UI controls.

### Blueprint Content

The project also contains supporting Blueprint assets under `Content/ProductConfig/Blueprints`, including:

- `BPAC_CameraMotion`
- `BPI_RuntimeAction`
- `BP_ConfigController`
- `BP_ConfigGameMode`
- `BP_Configurator`
- `STRUCT_VarSet`

The repository also contains UMG assets for the catalog and runtime presentation UI.

## Architecture

```text
Car_Showcase
│
├── C++ Runtime
│   └── APremiumGarageController
│       ├── Vehicle state
│       ├── Vehicle selection
│       ├── Camera control
│       ├── Material control
│       ├── Showroom control
│       ├── Transitions
│       └── Orbital camera
│
├── Runtime UI
│   └── UPremiumGarageWidget
│       ├── Vehicle selection
│       ├── Camera controls
│       ├── Paint controls
│       ├── RGB sliders
│       └── Information panel
│
└── Content
    ├── ProductAssets
    │   ├── Vehicle assets
    │   ├── Materials
    │   ├── Background / HDRI assets
    │   └── Variant sets
    │
    └── ProductConfig
        ├── Blueprints
        └── UMG
```

## Project Structure

```text
CarShowCase/
├── Config/
├── Content/
│   ├── ProductAssets/
│   ├── ProductConfig/
│   └── autos/
├── Source/
│   └── Car_Showcase/
│       ├── Car_Showcase.Build.cs
│       ├── Car_Showcase.cpp
│       ├── PremiumGarageController.h
│       └── PremiumGarageController.cpp
├── Car_Showcase.uproject
└── README.md
```

## Technologies

| Technology | Usage |
|---|---|
| Unreal Engine 5.7 | Real-time 3D runtime and editor |
| C++ | Runtime controller and interaction logic |
| UMG | Runtime interface and widgets |
| Slate | Programmatic UI construction and styling |
| Variant Manager | Product/vehicle variant configuration |

The C++ module uses Unreal's `Core`, `CoreUObject`, `Engine`, `InputCore`, `UMG`, `Slate`, and `SlateCore` modules.

## Interaction

The project is designed around mouse-driven runtime UI interaction. The controller enables the mouse cursor and uses a `GameAndUI` input mode so the interface can be used while the 3D showroom remains active.

The main interaction flow is:

1. Select a vehicle from the catalog.
2. Select an inspection view.
3. Inspect the vehicle from predefined or orbital camera positions.
4. Change the vehicle paint using presets or RGB controls.
5. Adjust the showroom environment color.
6. Reset the vehicle material when required.

## Running

Requirements:

- Unreal Engine 5.7
- A C++ build environment supported by Unreal Engine 5.7

Open `Car_Showcase.uproject`, allow Unreal Engine to generate/build the project files, compile the `Car_Showcase` module, and launch the configured showroom level.

## Project Status

Functional interactive visualization prototype.

The project is currently maintained as a portfolio-oriented Unreal Engine project demonstrating C++ runtime programming, interactive 3D presentation, camera systems, material manipulation, and UI integration.

## Development Notes

The project intentionally keeps the main presentation logic concentrated in `APremiumGarageController`. This makes the repository easy to inspect while keeping the runtime flow explicit: the controller owns the active vehicle and presentation state, while the runtime widget provides the user-facing controls.

The repository contains a substantial set of Unreal assets under `Content/`. The C++ source is comparatively small and is focused on connecting those assets into a configurable interactive presentation.

---

# Español

## Descripción

CarShowCase es un showroom interactivo de vehículos desarrollado con Unreal Engine 5.7. El proyecto está orientado a la visualización de producto en 3D en tiempo real, permitiendo seleccionar diferentes vehículos, inspeccionarlos mediante cámaras predefinidas o una vista orbital y modificar materiales y colores durante la ejecución.

No está planteado como un videojuego tradicional, sino como una experiencia interactiva de presentación de producto.

## Enfoque técnico

- Desarrollo con Unreal Engine 5.7
- Programación runtime en C++
- Visualización interactiva de producto en 3D
- Control de cámaras y estado de escena
- Modificación de materiales en runtime
- Interfaces UMG/Slate
- Integración de Variant Manager

## Funcionalidades

- Selección entre cinco vehículos configurados en el showroom.
- Cámaras para frente, rines, parte trasera/spoiler y motor.
- Vista orbital alrededor del vehículo seleccionado.
- Cambio de color de carrocería mediante materiales dinámicos.
- Presets de color y ajuste RGB mediante sliders.
- Cambio del color del entorno/showroom.
- Restauración de materiales originales.
- Transiciones al cambiar de vehículo.
- Panel de información asociado al vehículo y vista seleccionada.

## Implementación

`APremiumGarageController` concentra la lógica principal de la experiencia: selección y visibilidad de vehículos, cámaras, materiales, colores del showroom, transiciones y actualización de la interfaz.

`UPremiumGarageWidget` implementa la interfaz de runtime y genera los controles utilizados para seleccionar vehículos, cambiar vistas, modificar colores y consultar información.

El proyecto complementa el código C++ con Blueprints y assets de UMG almacenados en `Content/ProductConfig`.

## Estado

Prototipo funcional de visualización interactiva.

---

## Credits / Assets

The repository contains Unreal Engine assets and third-party/model content used by the project. Asset ownership and licensing should be checked individually before redistribution or commercial use.

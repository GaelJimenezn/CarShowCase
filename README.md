# CarShowCase

Unreal Engine 5.7 interactive showroom project.

Built with C++ and UMG/Slate.

The project is a showroom prototype for real-time 3D presentation.

## Features

- Model selection
- Inspection camera presets
- Orbital inspection mode
- Runtime color customization
- Material reset
- Showroom color controls
- Runtime UI controls
- Scene transitions

## Implementation

The main C++ system is the Premium Garage Controller. It coordinates the active scene, cameras, materials, transitions, and showroom state.

A runtime UI widget provides the controls used by the presentation.

```text
Premium Garage Controller
├── Scene state
├── Model selection
├── Camera control
├── Material control
├── Transitions
└── Orbital view

Runtime UI
├── Selection controls
├── Camera controls
├── Color controls
└── Information panel
```

## Source

```text
Source/Car_Showcase/
├── PremiumGarageController.h
├── PremiumGarageController.cpp
└── Car_Showcase.Build.cs
```

## Status

Functional interactive visualization prototype.
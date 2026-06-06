# ChibiOS OOP Code Generator Manual

The `codegen/` directory contains the XML model for the common OOP layer. This
manual also documents the shared C OOP generator used by OOP, VFS, VNS, XHAL,
EX, and several complex drivers.

The generator turns XML module descriptions into C headers, C sources, and, when
requested, hand-editable implementation include files. The generated code is
ordinary C: classes are structures with VMT pointers, interfaces are structures
with interface VMT pointers and an `instance_offset`, and public methods are C
functions or inline wrappers.

## Generator Location

The generator is implemented by the FMPP templates:

- `tools/ftl/processors/ccode/module.h.ftl`
- `tools/ftl/processors/ccode/module.c.ftl`
- `tools/ftl/processors/ccode/module_impl.inc.ftl`
- `tools/ftl/libs/libccode.ftl`
- `tools/ftl/libs/libcclasses.ftl`

Each subsystem normally provides:

- `codegen/config.fmpp`
- `codegen/modules.xml`
- one XML file per generated module

Run generation from the subsystem `codegen` directory:

```sh
fmpp -C config.fmpp
```

## Configuration File

A typical `config.fmpp` selects the shared processor and writes output relative
to the subsystem:

```yaml
sourceRoot: ../../../tools/ftl/processors/ccode
outputRoot: ..
dataRoot: .

freemarkerLinks: {
    ftllibs: ../../../tools/ftl/libs
}

data : {
  xml:xml (
    modules.xml
    {
    }
  )
}
```

The exact relative paths depend on the subsystem depth. For example, VFS and EX
use the same processor but different relative paths to reach `tools/ftl`.

## Module List

`modules.xml` is the root document loaded by FMPP. It includes the modules to be
generated and optional search paths for imports.

```xml
<instance>
  <paths>
    <path>../../common/oop/codegen</path>
  </paths>
  <modules>
    &oop_base_object;
    &oop_base_interface;
    &my_module;
  </modules>
</instance>
```

The `paths` entries are used by `<import>` resolution. The generator first looks
for an imported XML file in the current `dataRoot`, then in each listed path.

Order matters. Ancestors and imported interfaces/classes must be discoverable
when a module is initialized. Keep base modules before derived modules in
`modules.xml`, and add explicit `<import>` entries in each module that derives
from or implements types in another module.

## Module Element

A module describes one generated header and, when code is emitted, one generated
source.

```xml
<module name="hal_pwm" descr="PWM Driver" check="HAL_USE_PWM == TRUE"
  headerpath="include" sourcepath="src" editcode="false">
```

Important attributes:

- `name`: base name for generated files. `hal_pwm` generates `hal_pwm.h` and
  `hal_pwm.c`.
- `descr`: short textual description used in generated Doxygen comments.
- `headerpath`: output directory for the header, relative to `outputRoot`.
  Defaults to `include`.
- `sourcepath`: output directory for the source, relative to `outputRoot`.
  Defaults to `src`.
- `check`: optional preprocessor condition around most generated content.
- `editcode`: must be set explicitly. Use `false` for fully generated source.
  Use `true` when generated source includes a hand-editable `_impl.inc` file.

Use `editcode="false"` for generated-only modules such as many OOP/XHAL core
modules. Use `editcode="true"` for modules requiring preserved handwritten
implementation code, such as VFS filesystem drivers.

## Public And Private Sections

Each module has a `public` section and a `private` section.

```xml
<public>
  <includes>
  </includes>
  <definitions_early>
  </definitions_early>
  <configs>
  </configs>
  <definitions_late>
  </definitions_late>
  <macros>
  </macros>
  <types>
  </types>
  <variables>
  </variables>
  <functions>
  </functions>
</public>
<private>
  <includes>
  </includes>
  <definitions>
  </definitions>
  <macros>
  </macros>
  <types>
  </types>
  <variables>
  </variables>
  <functions>
  </functions>
</private>
```

Public content is emitted into the generated header. Private content is emitted
into the generated source.

If the private section does not specify `includes` or `includes_always`, the
generated source automatically includes its own generated header:

```c
#include "module_name.h"
```

`includes_always` is emitted before a module-level `check`. Normal `includes`
are emitted inside the module-level `check`.

## Includes

Use `style="regular"` for quoted includes and `style="angular"` for system
includes.

```xml
<includes>
  <include style="regular">oop_base_interface.h</include>
  <include style="angular">stdint.h</include>
</includes>
```

Conditions can be nested:

```xml
<condition check="defined(OOP_USE_CHIBIOS)">
  <include style="regular">ch.h</include>
  <elseif check="defined(OOP_USE_NOTHING)" />
  <include style="angular">stddef.h</include>
  <elseif />
  <include style="regular">osal.h</include>
</condition>
```

The generator emits preprocessor conditionals for these nodes. Keep related
`condition` and `elseif` blocks adjacent.

## Defines

Use `definitions_early` for constants that must be available before config
checks and type declarations. Use `definitions_late` for derived constants after
config checks.

```xml
<definitions_early>
  <group description="Common driver states">
    <define name="HAL_DRV_STATE_STOP" value="1U" />
    <define name="HAL_DRV_STATE_READY" value="4U" />
  </group>
</definitions_early>
```

Function-like defines are created by adding `<param>` elements:

```xml
<define name="VFS_MODE_S_ISDIR" value="S_ISDIR(mode)">
  <param name="mode" />
</define>
```

Use `verbatim` for raw preprocessor text:

```xml
<verbatim><![CDATA[
#undef STM_OK
#undef STM_TIMEOUT
]]></verbatim>
```

Groups create Doxygen `@name` sections.

## Configuration Options

Configurations generate guarded default defines plus validation checks:

```xml
<configs>
  <config name="HAL_USE_REGISTRY" default="TRUE">
    <brief>Enables the HAL registry for drivers.</brief>
    <note>Disabling this option saves both code and data space.</note>
    <assert invalid="($N != FALSE) &amp;&amp; ($N != TRUE)" />
  </config>
</configs>
```

`$N` is replaced by the config name in assertions.

Generated form:

```c
#if !defined(HAL_USE_REGISTRY) || defined(__DOXYGEN__)
#define HAL_USE_REGISTRY                         TRUE
#endif

#if (HAL_USE_REGISTRY != FALSE) && (HAL_USE_REGISTRY != TRUE)
#error "invalid HAL_USE_REGISTRY value"
#endif
```

Use config nodes for module-local options that belong in the generated public
header. For generated project configuration files such as `xhalconf.h`, update
their own templates as required by the subsystem rules.

## Macros

Use `<macros>` for multi-line macros with documented parameters and an
implementation:

```xml
<macro name="oopGetOwner">
  <brief>Returns an object pointer starting from a pointer to a class
    member.</brief>
  <param name="c">Class type name.</param>
  <param name="m">Class member field name.</param>
  <param name="p" dir="in">Class member field pointer.</param>
  <return>A pointer to an object of type @p c.</return>
  <api />
  <implementation><![CDATA[(c *)(((size_t)(p)) - (size_t)offsetof(c, m))]]></implementation>
</macro>
```

The generated macro body is line-wrapped with continuation backslashes.

## Types

The generator supports typedefs, structs, unions, classes, interfaces, raw
verbatim C, and conditional type blocks.

### Typedefs

```xml
<typedef name="driver_state_t">
  <brief>Type of a driver state variable.</brief>
  <basetype ctype="unsigned int" />
</typedef>
```

If the `basetype` contains `$N`, it is replaced with the typedef name.

```xml
<typedef name="my_callback_t">
  <basetype ctype="void (*$N)(void *ip)" />
</typedef>
```

### Structs And Unions

```xml
<struct name="vfs_stat">
  <brief>Structure representing a node information.</brief>
  <fields>
    <field name="mode" ctype="vfs_mode_t">
      <brief>Node mode.</brief>
    </field>
    <field name="size" ctype="vfs_offset_t">
      <brief>Size of the node.</brief>
    </field>
  </fields>
</struct>
```

Field declarations support `$I` and `$N` tokens. `$I` marks the identifier
insertion point, and `$N` is replaced by the field name.

Examples:

- `const void$I*` with field `config` becomes `const void *config`.
- `char$I$N[VFS_CFG_NAMELEN_MAX + 1]` with field `name` becomes
  `char name[VFS_CFG_NAMELEN_MAX + 1]`.
- `hal_regent_t$I*` with field `next` becomes `hal_regent_t *next`.

Use these tokens for pointers, arrays, function pointers, and other C
declarations where the identifier is not simply appended after the type.

## Interfaces

An interface is a pure method table plus a small interface object.

```xml
<interface name="sequential_stream" namespace="stm"
  ancestorname="base_interface" descr="sequential stream">
  <brief>Sequential data streams interface.</brief>
  <methods>
    <method name="stmWrite" shortname="write" ctype="size_t">
      <param name="bp" ctype="const uint8_t *" dir="in" />
      <param name="n" ctype="size_t" dir="in" />
      <return>The number of bytes transferred.</return>
    </method>
  </methods>
</interface>
```

Naming rules:

- `name="sequential_stream"` generates `sequential_stream_i`.
- The VMT type is `struct sequential_stream_vmt`.
- The instance structure is `struct sequential_stream`.
- `shortname="write"` generates the VMT slot `write`.
- `name="stmWrite"` generates the public inline wrapper `stmWrite()`.
- `namespace="stm"` is used when a class implements the interface and becomes
  the interface field name in the class.

Generated interface VMTs always contain:

```c
size_t instance_offset;
```

That offset lets the interface implementation recover the owning class object.

Interface inheritance is flattened into the generated VMT. A derived interface
contains all ancestor method slots followed by its own method slots. For example,
the EX `ex_accelerometer_i` interface extends `ex_calibrated_sensor_i`, which
extends `ex_sensor_i`; the generated accelerometer VMT contains sensor slots,
calibration slots, and then accelerometer slots.

Do not duplicate inherited methods in a derived interface just to preserve old
API names. Add compatibility defines instead:

```xml
<definitions_early>
  <group description="Legacy accelerometer inherited method names">
    <define name="accelerometerReadRaw" value="sensorReadRaw" />
    <define name="accelerometerSetBias" value="calibratedSensorSetBias" />
  </group>
</definitions_early>
```

## Classes

A class is a C structure with a VMT pointer, inherited fields, implemented
interface fields, and its own fields.

```xml
<class type="abstract" name="hal_base_driver" namespace="drv"
  ancestorname="base_object" descr="HAL base driver">
  <fields>
    <field name="state" ctype="driver_state_t" />
    <field name="config" ctype="const void$I*" />
  </fields>
  <methods>
    <objinit callsuper="true">
      <implementation><![CDATA[
self->state  = HAL_DRV_STATE_STOP;
self->config = NULL;]]></implementation>
    </objinit>
    <dispose>
      <implementation><![CDATA[ ]]></implementation>
    </dispose>
    <virtual>
      <method name="__drv_start" shortname="start" ctype="msg_t">
        <param ctype="const void *" name="config" dir="in" />
        <notapi />
      </method>
    </virtual>
  </methods>
</class>
```

Naming rules:

- `name="hal_base_driver"` generates `hal_base_driver_c`.
- The VMT type is `struct hal_base_driver_vmt`.
- The instance structure is `struct hal_base_driver`.
- `namespace="drv"` is used for generated internal helpers and constructors.
- `shortname="start"` generates the VMT slot `start`.

Class inheritance is flattened into the generated structure and VMT:

- Ancestor virtual method slots appear before current class slots.
- Ancestor implemented interfaces and fields appear before current class
  interfaces and fields.
- The generated object initialization helper can call the ancestor initializer.
- The generated dispose helper calls the ancestor disposer after current
  finalization code.

### Class Types

Use `type="abstract"` for classes that have no public default constructor and no
public VMT instance. Use `type="regular"` for instantiable classes.

For regular classes, the generator emits:

- `extern const struct <class>_vmt __<class>_vmt` in the inline constructor.
- a default constructor named `<namespace>ObjectInit()`.
- a generated VMT initializer in the source.

## Class Methods

Class methods are grouped under `<methods>`.

### Object Initialization

Every class should define `<objinit>`.

```xml
<objinit callsuper="true">
  <param name="config" ctype="const void *" dir="in" />
  <implementation><![CDATA[
self->config = config;]]></implementation>
</objinit>
```

The generated implementation has parameters:

```c
void *__<namespace>_objinit_impl(void *ip, const void *vmt, ...)
```

If `callsuper="true"`, the ancestor init helper is called before the local
implementation. If `callsuper="false"`, the local implementation is responsible
for any required ancestor initialization. VFS uses this when a derived node wants
to call a specific ancestor helper with customized parameters.

### Dispose

Every class should define `<dispose>`.

```xml
<dispose>
  <implementation><![CDATA[
/* local cleanup */
]]></implementation>
</dispose>
```

The generated dispose helper runs local finalization and then calls the ancestor
dispose helper if the class has an ancestor.

### Virtual Methods

Virtual methods define VMT slots and inline wrappers.

```xml
<virtual>
  <method name="__drv_start" shortname="start" ctype="msg_t">
    <param name="config" ctype="const void *" dir="in" />
    <notapi />
  </method>
</virtual>
```

If a virtual method has an `<implementation>`, the generator emits a default
implementation helper named:

```c
__<namespace>_<shortname>_impl()
```

If it has no implementation, the generated VMT initializer for a regular class
must find an override in a derived class or the method slot becomes a missing
implementation marker.

### Overrides

Use `<override>` to override a virtual method declared by an ancestor.

```xml
<override>
  <method shortname="start">
    <implementation><![CDATA[
return my_lld_start(self, config);]]></implementation>
  </method>
</override>
```

The override method is matched by `shortname` against ancestor virtual methods.
The generated C prototype is taken from the ancestor declaration, so the override
does not need to repeat return type or parameters.

XHAL drivers use this pattern to override base driver methods such as
`start`, `stop`, `setcfg`, and `selcfg`.

### Regular Methods

Regular methods are exported C functions with a `void *ip` object pointer as
their first parameter.

```xml
<regular>
  <method name="drvStart" ctype="msg_t">
    <param name="config" ctype="const void *" dir="in" />
    <api />
    <implementation><![CDATA[
/* method body */
]]></implementation>
  </method>
</regular>
```

The generated prototype appears in the header, and the implementation appears in
the source.

### Inline Methods

Inline methods are emitted in the header as `CC_FORCE_INLINE static inline`
functions.

```xml
<inline>
  <method name="drvGetStateX" ctype="driver_state_t">
    <implementation><![CDATA[
return self->state;]]></implementation>
  </method>
</inline>
```

Use inline methods for small accessors or wrappers where the API is intentionally
header-only.

## Implemented Interfaces

Classes can embed and implement interfaces.

```xml
<implements>
  <if name="random_stream">
    <method shortname="write">
      <implementation><![CDATA[
msg_t msg;

msg = vfsFileWrite((void *)self, bp, n);
if (CH_RET_IS_ERROR(msg)) {

  return (size_t)0;
}

return (size_t)msg;]]></implementation>
    </method>
  </if>
</implements>
```

This generates:

- an interface field in the class structure, named from the interface namespace.
- a static interface VMT inside the class initializer.
- VMT entries for all methods in the interface and its ancestors.
- one static adapter function per implemented method.

The adapter implementation receives the interface pointer as `ip`, but the
generator creates:

```c
<class>_c *self = oopIfGetOwner(<class>_c, ip);
```

The implementation body can then use `self` as the owning class pointer.

Every interface method in the flattened interface method list must have a
matching `<method shortname="...">` inside the `<if>` block. Missing entries
generate `NULL /* Missing implementation.*/` in the interface VMT.

VFS file nodes are a practical example: `vfs_file_node` implements
`random_stream` by adapting stream methods to file-node methods such as
`vfsFileRead()`, `vfsFileWrite()`, and `vfsFileSetPosition()`.

Conditional interface implementation is supported:

```xml
<implements>
  <condition check="SIO_USE_STREAMS_INTERFACE == TRUE">
    <if name="sequential_stream">
      ...
    </if>
  </condition>
</implements>
```

XHAL SIO uses this pattern to expose stream interfaces only when enabled by
configuration.

## Editable Implementation Mode

Set `editcode="true"` when a module needs generated declarations but preserved
handwritten implementation bodies.

In this mode:

- the generated `.c` includes `<module>_impl.inc`.
- `module_impl.inc.ftl` creates `<module>_impl.inc` only if it does not already
  exist.
- future generation does not overwrite the existing `_impl.inc`.

VFS filesystem drivers use this mode:

```xml
<module name="vfs_driver_romfs" descr="VFS ROMFS Driver"
  check="VFS_CFG_ENABLE_DRV_ROMFS == TRUE" sourcepath="drivers/romfs"
  headerpath="drivers/romfs" editcode="true">
```

Use this for driver templates or modules where the generated method skeleton is
useful but actual implementation must be maintained manually.

Do not use editable implementation mode as a shortcut for generated-only
modules. If the XML has complete implementations, use `editcode="false"` and let
the generator own the `.c` file.

## Conditions

Many XML sections accept `<condition check="...">`.

The generator emits:

```c
#if (<condition>) || defined (__DOXYGEN__)
...
#endif /* <condition> */
```

Module-level `check` wraps most generated content in both header and source.
This is used heavily in XHAL:

```xml
<module name="hal_pwm" descr="PWM Driver" check="HAL_USE_PWM == TRUE"
  editcode="false">
```

Field-level and method-level conditions are useful for optional features:

```xml
<condition check="ADC_USE_SYNCHRONIZATION == TRUE">
  <field name="sync_transfer" ctype="thread_reference_t" />
</condition>
```

Use conditions in XML when the optional shape changes generated declarations,
VMTs, fields, or API bodies. Do not hide only an implementation detail in XML if
normal C preprocessor logic inside an implementation block is clearer.

## Documentation Tags

The generator emits Doxygen from XML nodes. Use documentation elements
consistently:

- `brief`
- `details`
- `pre`
- `post`
- `note`
- `param`
- `return`
- `retval`
- `api`
- `notapi`
- `objinit`
- `dispose`

Documentation is not cosmetic here. Missing XML documentation can make generated
Doxygen comments incomplete or misleading.

Use `api` for public APIs, `notapi` for generated helpers or low-level virtuals
that are not part of the public API contract.

## C Declaration Tokens

The generator provides two important placeholders in `ctype` strings:

- `$I`: identifier insertion point, used to control spacing and pointer/array
  layout.
- `$N`: replacement for the generated name.

Examples:

```xml
<field name="config" ctype="const void$I*" />
<field name="name" ctype="char$I$N[VFS_CFG_NAMELEN_MAX + 1]" />
<basetype ctype="void (*$N)(void *ip)" />
```

Prefer these tokens whenever a declaration is not a simple `type name` form.

## Generated File Ownership

Generated headers and generated sources are derived files. Edit XML first, then
regenerate. Do not manually patch generated files unless the change is temporary
debugging work that will be discarded or moved back to XML before commit.

For a generated-only module:

- edit `<module>.xml`
- run `fmpp -C config.fmpp`
- commit XML and generated output together

For an editable implementation module:

- edit XML for declarations, object shape, and generated skeleton changes
- edit the existing `_impl.inc` for preserved implementation code
- regenerate to refresh `.h` and `.c`

## Practical Examples

### OOP Base Object

`oop_base_object.xml` defines the root abstract class. It shows:

- conditional includes for different OOP environments
- documented macros such as `oopGetOwner`
- root class initialization with `callsuper="false"`
- virtual disposal through `boDispose`
- an inline helper `boFree`

Use this as the reference for minimal root-class structure.

### OOP Streams

`oop_sequential_stream.xml` defines a pure interface. It shows:

- interface inheritance from `base_interface`
- legacy compatibility defines under a condition
- method `shortname` values that become VMT slots
- optional legacy typedefs

Use this as the reference for interface-only modules.

### VFS Nodes

`vfs_nodes.xml` defines abstract classes and interface implementations. It
shows:

- typedefs and structs emitted before classes
- class inheritance through `ancestorname`
- interface adaptation through `<implements>`
- custom ancestor initialization with `callsuper="false"`
- default virtual implementations returning `CH_RET_ENOSYS`

Use this as the reference for class hierarchies that expose stream interfaces.

### VFS Drivers

VFS filesystem driver XML files use `editcode="true"` plus custom `headerpath`
and `sourcepath`. They show how to generate stable public declarations while
preserving hand-edited implementation includes.

Use these as the reference for modules that should not overwrite implementation
code on regeneration.

### XHAL Base Driver

`hal_base_driver.xml` defines the shared XHAL driver class. It shows:

- generated configuration options and asserts
- fields controlled by feature switches
- virtual low-level hooks
- regular public lifecycle methods
- inline state/query helpers

Use this as the reference for stateful driver base classes.

### XHAL Peripheral Drivers

XHAL peripheral driver XML files such as `hal_adc.xml`, `hal_spi.xml`, and
`hal_sio.xml` show:

- module-level feature checks such as `HAL_USE_SPI == TRUE`
- optional fields and methods under feature switches
- virtual method overrides of `hal_base_driver`
- conditional interface implementation, for example stream interfaces
- complete generated public API bodies

Use these as the reference for generated HAL driver APIs.

### EX Interfaces

The new EX codegen area uses the same generator for isolated interface work. It
shows:

- generated output redirected with `headerpath="if"`
- a singular base interface `ex_sensor`
- an intermediate calibrated interface `ex_calibrated_sensor`
- legacy API names preserved with compatibility defines
- generated output kept separate from legacy `os/ex/include`

Use this as the reference for migrating old duplicated interfaces into a cleaner
interface hierarchy.

## Regeneration Workflow

Use this workflow for normal changes:

```sh
cd <subsystem>/codegen
fmpp -C config.fmpp
```

Then validate from the repository root:

```sh
xmllint --noout <subsystem>/codegen/modules.xml <subsystem>/codegen/*.xml
git diff --check -- <subsystem>
```

For large changes, also inspect generated names:

```sh
rg -n "struct .*_vmt|static inline|#include" <subsystem>
```

For interface or class hierarchy changes, explicitly inspect:

- generated include order
- generated structure field order
- generated VMT slot order
- inherited method slots
- compatibility defines
- missing implementation markers

## Common Mistakes

Avoid these patterns:

- editing generated headers or generated sources instead of XML
- forgetting to add `<import>` for an ancestor or implemented interface
- relying on implicit `editcode` behavior instead of setting it explicitly
- duplicating inherited interface methods in a derived interface
- changing a method `shortname` without checking VMT slot compatibility
- adding fields under conditions without checking object layout implications
- forgetting that derived interface VMTs include inherited slots
- leaving stale generated files after a module rename
- omitting documentation nodes needed by generated Doxygen
- using ad hoc string declarations instead of `$I` and `$N` where required

## Review Checklist

Before committing an OOP generator change:

- XML parses with `xmllint`.
- `fmpp -C config.fmpp` runs without warnings.
- Generated files match the XML intent.
- No generated file still carries a stale module name after renames.
- Public includes point to generated headers that exist.
- Inherited VMT slots are intentional and in the expected order.
- Classes implementing interfaces provide all required method adapters.
- `git diff --check` is clean.
- Hand-editable `_impl.inc` files were not overwritten accidentally.

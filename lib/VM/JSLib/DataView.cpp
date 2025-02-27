/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */
#include "JSLibInternal.h"

#include "hermes/VM/JSDataView.h"
#include "hermes/VM/JSTypedArray.h"
#include "hermes/VM/StringPrimitive.h"

namespace hermes {
namespace vm {

namespace {

/// @name DataView.prototype
/// @{

// ES6 24.2.4.1
CallResult<HermesValue>
dataViewPrototypeBuffer(void *, Runtime *runtime, NativeArgs args) {
  auto self = args.dyncastThis<JSDataView>(runtime);
  if (!self) {
    return runtime->raiseTypeError(
        "DataView.prototype.buffer called on a non DataView object");
  }
  return self->getBuffer(runtime).getHermesValue();
}

// ES6 24.2.4.2
CallResult<HermesValue>
dataViewPrototypeByteLength(void *, Runtime *runtime, NativeArgs args) {
  auto self = args.dyncastThis<JSDataView>(runtime);
  if (!self) {
    return runtime->raiseTypeError(
        "DataView.prototype.byteLength called on a non DataView object");
  }
  return HermesValue::encodeNumberValue(self->byteLength());
}

// ES6 24.2.4.3
CallResult<HermesValue>
dataViewPrototypeByteOffset(void *, Runtime *runtime, NativeArgs args) {
  auto self = args.dyncastThis<JSDataView>(runtime);
  if (!self) {
    return runtime->raiseTypeError(
        "DataView.prototype.byteOffset called on a non DataView object");
  }
  return HermesValue::encodeNumberValue(self->byteOffset());
}

// ES6 24.2.4.5 - 22.2.4.20 && ES 2018 24.3.1.1
template <typename T>
CallResult<HermesValue>
dataViewPrototypeGet(void *, Runtime *runtime, NativeArgs args) {
  auto self = args.dyncastThis<JSDataView>(runtime);
  if (!self) {
    return runtime->raiseTypeError(
        "DataView.prototype.get<Type>() called on a non DataView object");
  }
  auto res = toIndex(runtime, args.getArgHandle(runtime, 0));
  if (res == ExecutionStatus::EXCEPTION) {
    return ExecutionStatus::EXCEPTION;
  }
  auto byteOffset = res->getNumberAs<uint64_t>();
  auto littleEndian = toBoolean(args.getArg(1));
  if (!self->attached(runtime)) {
    return runtime->raiseTypeError(
        "DataView.prototype.get<Type>() called on a detached ArrayBuffer");
  }
  if (byteOffset + sizeof(T) > self->byteLength()) {
    return runtime->raiseRangeError(
        "DataView.prototype.get<Type>(): Cannot "
        "read that many bytes");
  }
  return SafeNumericEncoder<T>::encode(
      self->get<T>(runtime, byteOffset, littleEndian));
}

template <typename T, CellKind C>
CallResult<HermesValue>
dataViewPrototypeSet(void *, Runtime *runtime, NativeArgs args) {
  auto self = args.dyncastThis<JSDataView>(runtime);
  if (!self) {
    return runtime->raiseTypeError(
        "DataView.prototype.set<Type>() called on a non DataView object");
  }
  auto res = toIndex(runtime, args.getArgHandle(runtime, 0));
  if (res == ExecutionStatus::EXCEPTION) {
    return ExecutionStatus::EXCEPTION;
  }
  auto byteOffset = res->getNumberAs<uint64_t>();
  auto littleEndian = toBoolean(args.getArg(2));
  res = toNumber_RJS(runtime, args.getArgHandle(runtime, 1));
  if (res == ExecutionStatus::EXCEPTION) {
    return ExecutionStatus::EXCEPTION;
  }
  if (!self->attached(runtime)) {
    return runtime->raiseTypeError(
        "DataView.prototype.set<Type> called on a detached ArrayBuffer");
  }
  T value = JSTypedArray<T, C>::toDestType(res->getNumber());
  if (byteOffset + sizeof(T) > self->byteLength()) {
    return runtime->raiseRangeError(
        "DataView.prototype.set<Type>(): Cannot "
        "write that many bytes");
  }
  self->set<T>(runtime, byteOffset, value, littleEndian);
  return HermesValue::encodeUndefinedValue();
}

/// @}

// ES 2018 24.3.2.1
CallResult<HermesValue>
dataViewConstructor(void *, Runtime *runtime, NativeArgs args) {
  // 1. If NewTarget is undefined, throw a TypeError exception.
  if (!args.isConstructorCall()) {
    return runtime->raiseTypeError(
        "DataView() called in function context instead of constructor");
  }
  auto self = args.vmcastThis<JSDataView>();
  auto buffer = args.dyncastArg<JSArrayBuffer>(runtime, 0);
  auto byteLength = args.getArgHandle(runtime, 2);
  // 2. If Type(buffer) is not Object, throw a TypeError exception
  // 3. If buffer does not have an [[ArrayBufferData]] internal slot, throw a
  // TypeError exception.
  if (!buffer) {
    return runtime->raiseTypeError(
        "new DataView(buffer, [byteOffset], [byteLength]): "
        "buffer must be an ArrayBuffer");
  }

  // 4. Let offset be ToIndex(byteOffset).
  auto res = toIndex(runtime, args.getArgHandle(runtime, 1));
  if (res == ExecutionStatus::EXCEPTION) {
    return ExecutionStatus::EXCEPTION;
  }
  auto offset = res->getNumberAs<uint64_t>();
  // 6. Let bufferByteLength be buffer.[[ArrayBufferByteLength]].
  auto bufferByteLength = buffer->size();
  // 7. If offset > bufferByteLength, throw a RangeError exception.
  if (offset > bufferByteLength) {
    return runtime->raiseRangeError(
        "new DataView(buffer, [byteOffset], "
        "[byteLength]): byteOffset must be <= the "
        "buffer's byte length");
  }
  double viewByteLength = 0;
  // 8. If byteLength is either not present or undefined, then
  if (byteLength->isUndefined()) {
    // 8a. Let viewByteLength be bufferByteLength - offset.
    viewByteLength = bufferByteLength - offset;
  } else {
    // 9a. Let viewByteLength be ? ToIndex(byteLength).
    auto res2 = toIndex(runtime, byteLength);
    if (res2 == ExecutionStatus::EXCEPTION) {
      return ExecutionStatus::EXCEPTION;
    }
    viewByteLength = res2->getNumber();
    // 9b. If offset+viewByteLength > bufferByteLength, throw a RangeError
    // exception.
    if (offset + viewByteLength > bufferByteLength) {
      return runtime->raiseRangeError(
          "new DataView(buffer, [byteOffset], "
          "[byteLength]): byteOffset + byteLength must be "
          "<= the length of the buffer");
    }
  }
  self->setBuffer(runtime, *buffer, offset, viewByteLength);
  return self.getHermesValue();
}

} // namespace

Handle<JSObject> createDataViewConstructor(Runtime *runtime) {
  auto proto = Handle<JSObject>::vmcast(&runtime->dataViewPrototype);
  auto cons = defineSystemConstructor<JSDataView>(
      runtime,
      Predefined::getSymbolID(Predefined::DataView),
      dataViewConstructor,
      proto,
      3,
      CellKind::DataViewKind);

  // DataView.prototype.xxx() methods.
  defineAccessor(
      runtime,
      proto,
      Predefined::getSymbolID(Predefined::buffer),
      nullptr,
      dataViewPrototypeBuffer,
      nullptr,
      false,
      true);
  defineAccessor(
      runtime,
      proto,
      Predefined::getSymbolID(Predefined::byteLength),
      nullptr,
      dataViewPrototypeByteLength,
      nullptr,
      false,
      true);
  defineAccessor(
      runtime,
      proto,
      Predefined::getSymbolID(Predefined::byteOffset),
      nullptr,
      dataViewPrototypeByteOffset,
      nullptr,
      false,
      true);

#define TYPED_ARRAY(name, type)                              \
  defineMethod(                                              \
      runtime,                                               \
      proto,                                                 \
      Predefined::getSymbolID(Predefined::get##name),        \
      nullptr,                                               \
      dataViewPrototypeGet<type>,                            \
      1);                                                    \
  defineMethod(                                              \
      runtime,                                               \
      proto,                                                 \
      Predefined::getSymbolID(Predefined::set##name),        \
      nullptr,                                               \
      dataViewPrototypeSet<type, CellKind::name##ArrayKind>, \
      2);
#define TYPED_ARRAY_NO_CLAMP
#include "hermes/VM/TypedArrays.def"

  auto dpf = DefinePropertyFlags::getDefaultNewPropertyFlags();
  dpf.writable = 0;
  dpf.enumerable = 0;
  defineProperty(
      runtime,
      proto,
      Predefined::getSymbolID(Predefined::SymbolToStringTag),
      runtime->getPredefinedStringHandle(Predefined::DataView),
      dpf);

  // DataView.xxx() methods.

  return cons;
}

} // namespace vm
} // namespace hermes

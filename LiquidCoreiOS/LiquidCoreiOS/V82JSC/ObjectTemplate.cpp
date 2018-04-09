//
//  ObjectTemplate.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

/** Creates an ObjectTemplate. */
Local<ObjectTemplate> ObjectTemplate::New(
                                 Isolate* isolate,
                                 Local<FunctionTemplate> constructor)
{
    ObjectTemplateImpl *otempl = (ObjectTemplateImpl*) TemplateImpl::New(isolate, sizeof(ObjectTemplateImpl));
    otempl->pMap->set_instance_type(v8::internal::OBJECT_TEMPLATE_INFO_TYPE);

    FunctionTemplateImpl *ctor = nullptr;
    if (!constructor.IsEmpty()) {
        ctor = V82JSC::ToImpl<FunctionTemplateImpl>(constructor);
    }
    if (ctor) {
        otempl->m_constructor_template = ctor;
    }

    return _local<ObjectTemplate>(otempl).toLocal();
}

/** Get a template included in the snapshot by index. */
MaybeLocal<ObjectTemplate> ObjectTemplate::FromSnapshot(Isolate* isolate,
                                               size_t index)
{
    return MaybeLocal<ObjectTemplate>();
}

/** Creates a new instance of this template.*/
MaybeLocal<Object> ObjectTemplate::NewInstance(Local<Context> context)
{
    ObjectTemplateImpl *impl = V82JSC::ToImpl<ObjectTemplateImpl>(this);
    const ContextImpl *ctx = V82JSC::ToContextImpl(context);
    
    JSClassRef claz = JSClassCreate(&impl->m_definition);
    
    TemplateWrap *wrap = new TemplateWrap();
    wrap->m_template = impl;
    wrap->m_context = ctx;
    LocalException exception(ctx->isolate);
    
    JSObjectRef instance = 0;
    if (impl->m_constructor_template) {
        MaybeLocal<Function> ctor = _local<FunctionTemplate>(impl->m_constructor_template).toLocal()->GetFunction(context);
        if (!ctor.IsEmpty()) {
            JSValueRef ctor_func = V82JSC::ToJSValueRef(ctor.ToLocalChecked(), context);
            instance = (JSObjectRef) JSObjectCallAsConstructor(ctx->m_context, (JSObjectRef)ctor_func, 0, 0, &exception);
        }
    } else {
        instance = JSObjectMake(ctx->m_context, claz, (void*)wrap);
    }
    JSClassRelease(claz);
    if (!instance) {
        return MaybeLocal<Object>();
    }
    return impl->InitInstance(context, instance, exception);
}

/**
 * Sets an accessor on the object template.
 *
 * Whenever the property with the given name is accessed on objects
 * created from this ObjectTemplate the getter and setter callbacks
 * are called instead of getting and setting the property directly
 * on the JavaScript object.
 *
 * \param name The name of the property for which an accessor is added.
 * \param getter The callback to invoke when getting the property.
 * \param setter The callback to invoke when setting the property.
 * \param data A piece of data that will be passed to the getter and setter
 *   callbacks whenever they are invoked.
 * \param settings Access control settings for the accessor. This is a bit
 *   field consisting of one of more of
 *   DEFAULT = 0, ALL_CAN_READ = 1, or ALL_CAN_WRITE = 2.
 *   The default is to not allow cross-context access.
 *   ALL_CAN_READ means that all cross-context reads are allowed.
 *   ALL_CAN_WRITE means that all cross-context writes are allowed.
 *   The combination ALL_CAN_READ | ALL_CAN_WRITE can be used to allow all
 *   cross-context access.
 * \param attribute The attributes of the property for which an accessor
 *   is added.
 * \param signature The signature describes valid receivers for the accessor
 *   and is used to perform implicit instance checks against them. If the
 *   receiver is incompatible (i.e. is not an instance of the constructor as
 *   defined by FunctionTemplate::HasInstance()), an implicit TypeError is
 *   thrown and no callback is invoked.
 */
void ObjectTemplate::SetAccessor(
                 Local<String> name, AccessorGetterCallback getter,
                 AccessorSetterCallback setter, Local<Value> data,
                 AccessControl settings, PropertyAttribute attribute,
                 Local<AccessorSignature> signature)
{
    ObjectTemplateImpl *this_ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    ValueImpl *name_ = V82JSC::ToImpl<ValueImpl>(name);
    JSStringRef s = JSValueToStringCopy(name_->m_context->m_context, name_->m_value, 0);
    // FIXME: Deal with attributes
    // FIXME: Deal with AccessControl
    // FIXME: Deal with signature
    
    ObjAccessor accessor;
    accessor.m_getter = getter;
    accessor.m_setter = setter;
    if (!*data) {
        data = Undefined(Isolate::GetCurrent());
    }
    accessor.m_data = V82JSC::ToJSValueRef<Value>(data, Isolate::GetCurrent());
    JSValueProtect(V82JSC::ToIsolateImpl(Isolate::GetCurrent())->m_defaultContext->m_context, accessor.m_data);
    for (auto i = this_->m_obj_accessors.begin(); i != this_->m_obj_accessors.end(); ++i ) {
        if (JSStringIsEqual(i->first, s)) {
            i->second = accessor;
            JSStringRelease(s);
            return;
        }
    }
    this_->m_obj_accessors[s] = accessor;
}
void ObjectTemplate::SetAccessor(
                 Local<Name> name, AccessorNameGetterCallback getter,
                 AccessorNameSetterCallback setter, Local<Value> data,
                 AccessControl settings, PropertyAttribute attribute,
                 Local<AccessorSignature> signature)
{
    assert(0);
}

/**
 * Sets a named property handler on the object template.
 *
 * Whenever a property whose name is a string is accessed on objects created
 * from this object template, the provided callback is invoked instead of
 * accessing the property directly on the JavaScript object.
 *
 * SetNamedPropertyHandler() is different from SetHandler(), in
 * that the latter can intercept symbol-named properties as well as
 * string-named properties when called with a
 * NamedPropertyHandlerConfiguration. New code should use SetHandler().
 *
 * \param getter The callback to invoke when getting a property.
 * \param setter The callback to invoke when setting a property.
 * \param query The callback to invoke to check if a property is present,
 *   and if present, get its attributes.
 * \param deleter The callback to invoke when deleting a property.
 * \param enumerator The callback to invoke to enumerate all the named
 *   properties of an object.
 * \param data A piece of data that will be passed to the callbacks
 *   whenever they are invoked.
 */
// TODO(dcarney): deprecate
void ObjectTemplate::SetNamedPropertyHandler(NamedPropertyGetterCallback getter,
                             NamedPropertySetterCallback setter,
                             NamedPropertyQueryCallback query,
                             NamedPropertyDeleterCallback deleter,
                             NamedPropertyEnumeratorCallback enumerator,
                             Local<Value> data)
{
    
}

/**
 * Sets a named property handler on the object template.
 *
 * Whenever a property whose name is a string or a symbol is accessed on
 * objects created from this object template, the provided callback is
 * invoked instead of accessing the property directly on the JavaScript
 * object.
 *
 * @param configuration The NamedPropertyHandlerConfiguration that defines the
 * callbacks to invoke when accessing a property.
 */
void ObjectTemplate::SetHandler(const NamedPropertyHandlerConfiguration& configuration)
{
    
}

/**
 * Sets an indexed property handler on the object template.
 *
 * Whenever an indexed property is accessed on objects created from
 * this object template, the provided callback is invoked instead of
 * accessing the property directly on the JavaScript object.
 *
 * @param configuration The IndexedPropertyHandlerConfiguration that defines
 * the callbacks to invoke when accessing a property.
 */
void ObjectTemplate::SetHandler(const IndexedPropertyHandlerConfiguration& configuration)
{
    
}

/**
 * Sets the callback to be used when calling instances created from
 * this template as a function.  If no callback is set, instances
 * behave like normal JavaScript objects that cannot be called as a
 * function.
 */
void ObjectTemplate::SetCallAsFunctionHandler(FunctionCallback callback,
                              Local<Value> data)
{
    
}

/**
 * Mark object instances of the template as undetectable.
 *
 * In many ways, undetectable objects behave as though they are not
 * there.  They behave like 'undefined' in conditionals and when
 * printed.  However, properties can be accessed and called as on
 * normal objects.
 */
void ObjectTemplate::MarkAsUndetectable()
{
    
}

/**
 * Sets access check callback on the object template and enables access
 * checks.
 *
 * When accessing properties on instances of this object template,
 * the access check callback will be called to determine whether or
 * not to allow cross-context access to the properties.
 */
void ObjectTemplate::SetAccessCheckCallback(AccessCheckCallback callback,
                            Local<Value> data)
{
    
}

/**
 * Like SetAccessCheckCallback but invokes an interceptor on failed access
 * checks instead of looking up all-can-read properties. You can only use
 * either this method or SetAccessCheckCallback, but not both at the same
 * time.
 */
void ObjectTemplate::SetAccessCheckCallbackAndHandler(
                                      AccessCheckCallback callback,
                                      const NamedPropertyHandlerConfiguration& named_handler,
                                      const IndexedPropertyHandlerConfiguration& indexed_handler,
                                      Local<Value> data)
{
    
}

/**
 * Gets the number of internal fields for objects generated from
 * this template.
 */
int ObjectTemplate::InternalFieldCount()
{
    return 0;
}

/**
 * Sets the number of internal fields for objects generated from
 * this template.
 */
void ObjectTemplate::SetInternalFieldCount(int value)
{
    
}

/**
 * Returns true if the object will be an immutable prototype exotic object.
 */
bool ObjectTemplate::IsImmutableProto()
{
    return false;
}

/**
 * Makes the ObjectTempate for an immutable prototype exotic object, with an
 * immutable __proto__.
 */
void ObjectTemplate::SetImmutableProto()
{
    
}

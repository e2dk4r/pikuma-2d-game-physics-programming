# see https://lldb.llvm.org/use/variable.html#synthetic-children for documentation
import lldb

# include/text.h
def string_summary(valobj, dict):
    value = valobj.GetChildMemberWithName("value").GetValueAsUnsigned(0)
    length = valobj.GetChildMemberWithName("length").GetValueAsUnsigned(0)

    if value == 0:
        return "(null string)"
    if value != 0 and length == 0:
        return "(empty string)"

    process = valobj.GetProcess()
    error = lldb.SBError()
    raw_data = process.ReadMemory(value, length, error)

    if error.Success():
        return f'(length: {length}) "{raw_data.decode("utf-8", "ignore")}"'
    else:
        return "<error reading memory>"

# include/string_builder.h
def string_builder_summary(valobj, dict):
    length = valobj.GetChildMemberWithName("length").GetValueAsUnsigned(0)
    out_buffer_value = valobj.GetChildMemberWithName("outBuffer").GetChildMemberWithName("value").GetValueAsUnsigned(0)

    if length == 0 or out_buffer_value == 0:
        return "length: 0"

    process = valobj.GetProcess()
    error = lldb.SBError()
    raw_data = process.ReadMemory(out_buffer_value, length, error)

    if error.Success():
        return f'length: {length} "{raw_data.decode("utf-8", "ignore")}"'
    else:
        return "<error reading memory>"

# src/physics.h
def volume_summary(valobj, dict):
    # TODO: pretty print volume type
    volume_type = valobj.GetChildMemberWithName("type").GetValueAsUnsigned(0)
    volume_data_address = valobj.GetValueAsAddress() + valobj.GetType().GetByteSize()

    process = valobj.GetProcess()
    error = lldb.SBError()
    raw_data = process.ReadMemory(volume_data_address)
    
    VOLUME_TYPE_CIRCLE = (1 << 0)
    VOLUME_TYPE_POLYGON = (1 << 1)
    VOLUME_TYPE_BOX = (1 << 2)
    VOLUME_TYPE_TRIANGLE = (1 << 3)

    print(f"type {valobj.GetValueAsAddress()} data {volume_data}")
    if volume_type == VOLUME_TYPE_CIRCLE:
        radius = volume_data.GetChildMemberWithName("radius").GetData().GetFloat(0)
        return "circle r = {radius}"
    elif volume_type == VOLUME_TYPE_BOX:
        width = volume_data.GetChildMemberWithName("width").GetData().GetFloat(0)
        height = volume_data.GetChildMemberWithName("height").GetData().GetFloat(0)
        return "box width = {width} height = {height}"
    elif volume_type == VOLUME_TYPE_POLYGON:
        vertex_count = volume_data.GetChildMemberWithName("vertexCount").GetData().GetFloat(0)
        return "polygon {vertexCount}"

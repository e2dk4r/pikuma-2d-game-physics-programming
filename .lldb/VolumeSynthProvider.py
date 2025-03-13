# see https://lldb.llvm.org/use/variable.html#synthetic-children for documentation
# see src/physics.h for volume structure
# TODO: why we cannot pretty print volume with lldb?

class VolumeSynthProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.type = None
        print('init', self.type)

    def update(self):
        # Get the type of volume
        self.type = self.valobj.GetChildMemberWithName("type").GetValueAsUnsigned(0)

        # Dereference pointer to get width and height for box
        volume = self.valobj + 1
        print('updated', self.type)

        if self.type == 1:  # VOLUME_TYPE_CIRCLE
            # Dereference pointer to get the radius for circle
            self.radius = volume.GetChildMemberWithName("radius")
        elif self.type == 2:  # VOLUME_TYPE_POLYGON
            self.vertexCount = volume.GetChildMemberWithName("vertexCount")
        elif self.type == 3:  # VOLUME_TYPE_BOX
            self.width = volume.GetChildMemberWithName("width")
            self.height = volume.GetChildMemberWithName("height")

    def get_value(self):
        if self.type == 1:  # CIRCLE
            return f"CIRCLE {{ radius = {self.radius} }}"
        elif self.type == 2:  # POLYGON
            return f"POLYGON {{ vertexCount = {self.vertexCount} }}"
        elif self.type == 3:  # BOX
            return f"BOX {{ width = {self.width}, height = {self.height} }}"
        else:
            return "UNKNOWN VOLUME"



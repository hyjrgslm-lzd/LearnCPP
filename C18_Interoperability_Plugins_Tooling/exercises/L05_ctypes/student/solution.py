C18_STATUS_PLUGIN_ERROR = 6


class CtypesSession:
    def __init__(self, library: str):
        self.library = library

    def set_callback(self, callback):
        self.callback = callback

    def process_into(self, payload: bytes, output: bytearray):
        return C18_STATUS_PLUGIN_ERROR, 0

    def process(self, payload: bytes) -> bytes:
        raise RuntimeError("TODO: load c18_get_api with ctypes and call process")

    def callback_error(self) -> str:
        return ""

    def close(self):
        return 0

import esphome.codegen as cg
from esphome.types import ConfigType
from tests.testing_helpers import ComponentManifestOverride


def override_manifest(manifest: ComponentManifestOverride) -> None:
    # The replies the tests check; reply_test.cpp supplies the tables codegen emits
    async def to_code_testing(config: ConfigType) -> None:
        cg.add_define("HDMI_CEC_OSD_NAME_LENGTH", 7)
        cg.add_define("HDMI_CEC_CLAIMED_OPCODE_COUNT", 1)
        cg.add_define("USE_HDMI_CEC_FEATURE_ABORT")

    manifest.to_code = to_code_testing

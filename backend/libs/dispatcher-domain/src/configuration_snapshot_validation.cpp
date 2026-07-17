#include <dispatcher/domain/configuration_snapshot_validation.hpp>

namespace dispatcher::domain
{
    dispatcher::common::ValidationResult validate_configuration_snapshot(
        const ConfigurationSnapshot& snapshot
    )
    {
        dispatcher::common::ValidationResult result;

        if (snapshot.config_version() == 0)
        {
            result.add_error(
                "config_version",
                "config_version must be greater than zero"
            );
        }

        for (const auto& tag : snapshot.tag_catalog().tags())
        {
            const auto device = snapshot.device_catalog().find_by_id(tag.device_id());

            if (!device.has_value())
            {
                result.add_error(
                    "tag.device_id",
                    "tag references unknown device: " + tag.device_id().value()
                );
            }
        }

        return result;
    }
}
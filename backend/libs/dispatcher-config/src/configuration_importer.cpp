#include <dispatcher/config/configuration_importer.hpp>

#include <dispatcher/config/configuration_import_model_mapper.hpp>
#include <dispatcher/config/configuration_json_import_parser.hpp>

namespace dispatcher::config
{
    ConfigurationSnapshotImportResult ConfigurationImporter::import_document(
        const ConfigurationDocument& document
    )
    {
        const auto model_result =
            ConfigurationJsonImportParser::parse(document);

        if (model_result.failed())
        {
            return ConfigurationSnapshotImportResult::failure(
                model_result.status(),
                model_result.error().operation,
                model_result.error().resource,
                model_result.error().field,
                model_result.error().message
            );
        }

        return ConfigurationImportModelMapper::map(model_result.model());
    }
}
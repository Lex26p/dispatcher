using Dispatcher.Server.Configuration;
using Dispatcher.Server.Mimics;
using Dispatcher.Server.Templates;
using Microsoft.Data.Sqlite;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class MimicTemplateStoreTests
{
    [TestMethod]
    public async Task InitializeAsync_CreatesSchemaVersion9_AndRoundTripsMimicTemplate()
    {
        using var database =
            TemporaryDatabase.Create();
        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await store.InitializeAsync();
        await store.InitializeAsync();

        Assert.AreEqual(
            9,
            await ReadSchemaVersionAsync(
                database.DatabasePath));

        var template =
            CreateTemplate();

        await store.UpsertMimicTemplateAsync(
            template);

        var loaded =
            await store.LoadMimicTemplatesAsync();

        Assert.AreEqual(
            1,
            loaded.Count);
        var roundTripped =
            loaded[0];

        Assert.AreEqual(
            1,
            roundTripped.Version);
        Assert.AreEqual(
            template.TemplateId,
            roundTripped.TemplateId);
        Assert.AreEqual(
            template.Name,
            roundTripped.Name);
        Assert.AreEqual(
            template.Width,
            roundTripped.Width);
        Assert.AreEqual(
            template.Height,
            roundTripped.Height);
        Assert.AreEqual(
            1,
            roundTripped.Parameters.Count);
        Assert.AreEqual(
            "state",
            roundTripped.Parameters[0].ParameterId);
        Assert.AreEqual(
            2,
            roundTripped.Elements.Count);
        Assert.AreEqual(
            "state",
            roundTripped.Elements[1].TagParameterId);

        Assert.IsTrue(
            await store.DeleteMimicTemplateAsync(
                template.TemplateId));
        Assert.AreEqual(
            0,
            (await store.LoadMimicTemplatesAsync()).Count);
        Assert.AreEqual(
            0,
            (await store.LoadTemplateCatalogAsync()).Count);
    }

    [TestMethod]
    public async Task InitializeAsync_MigratesVersion7ToVersion9_WithoutChangingExistingMimic()
    {
        using var database =
            TemporaryDatabase.Create();

        await using (var connection =
            new SqliteConnection(
                CreateConnectionString(
                    database.DatabasePath)))
        {
            await connection.OpenAsync();

            await using var command =
                connection.CreateCommand();

            command.CommandText =
                """
                CREATE TABLE mimics (
                    mimic_id TEXT NOT NULL PRIMARY KEY,
                    name TEXT NOT NULL,
                    width INTEGER NOT NULL CHECK (width > 0),
                    height INTEGER NOT NULL CHECK (height > 0),
                    elements_json TEXT NOT NULL
                );

                INSERT INTO mimics (
                    mimic_id,
                    name,
                    width,
                    height,
                    elements_json)
                VALUES (
                    'existing',
                    'Existing mimic',
                    800,
                    600,
                    '[]');

                PRAGMA user_version = 7;
                """;

            await command.ExecuteNonQueryAsync();
        }

        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await store.InitializeAsync();

        Assert.AreEqual(
            9,
            await ReadSchemaVersionAsync(
                database.DatabasePath));

        var mimics =
            await store.LoadMimicsAsync();

        Assert.AreEqual(
            1,
            mimics.Count);
        Assert.AreEqual(
            "existing",
            mimics[0].MimicId);
        Assert.AreEqual(
            "Existing mimic",
            mimics[0].Name);

        await store.UpsertMimicTemplateAsync(
            CreateTemplate());

        Assert.AreEqual(
            1,
            (await store.LoadMimicTemplatesAsync()).Count);
    }


    [TestMethod]
    public async Task InitializeAsync_MigratesVersion8ToVersion9_AndMovesMimicMetadataIntoCatalog()
    {
        using var database =
            TemporaryDatabase.Create();

        await using (var connection =
            new SqliteConnection(
                CreateConnectionString(
                    database.DatabasePath)))
        {
            await connection.OpenAsync();
            await using var command =
                connection.CreateCommand();
            command.CommandText =
                """
                CREATE TABLE mimic_templates (
                    template_id TEXT NOT NULL PRIMARY KEY,
                    name TEXT NOT NULL,
                    width INTEGER NOT NULL CHECK (width > 0),
                    height INTEGER NOT NULL CHECK (height > 0),
                    parameters_json TEXT NOT NULL,
                    elements_json TEXT NOT NULL
                );

                INSERT INTO mimic_templates (
                    template_id,
                    name,
                    width,
                    height,
                    parameters_json,
                    elements_json)
                VALUES (
                    'legacy-mimic-template',
                    'Legacy mimic template',
                    320,
                    180,
                    '[{"ParameterId":"state","Name":"State tag"}]',
                    '[]');

                PRAGMA user_version = 8;
                """;

            await command.ExecuteNonQueryAsync();
        }

        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await store.InitializeAsync();

        Assert.AreEqual(
            9,
            await ReadSchemaVersionAsync(
                database.DatabasePath));

        var templates =
            await store.LoadMimicTemplatesAsync();
        var catalog =
            await store.LoadTemplateCatalogAsync();

        Assert.AreEqual(
            1,
            templates.Count);
        Assert.AreEqual(
            "legacy-mimic-template",
            templates[0].TemplateId);
        Assert.AreEqual(
            "Legacy mimic template",
            templates[0].Name);
        Assert.AreEqual(
            1,
            templates[0].Version);
        Assert.AreEqual(
            "state",
            templates[0].Parameters.Single().ParameterId);

        Assert.AreEqual(
            1,
            catalog.Count);
        Assert.AreEqual(
            TemplateKind.Mimic,
            catalog[0].Kind);
        Assert.AreEqual(
            1,
            catalog[0].Version);
        Assert.AreEqual(
            "state",
            catalog[0].Parameters.Single().ParameterId);
    }

    private static MimicTemplateConfiguration CreateTemplate()
    {
        return new MimicTemplateConfiguration(
            TemplateId:
                "pump-fragment",
            Name:
                "Pump fragment",
            Width:
                240,
            Height:
                120,
            Parameters:
            [
                new MimicTemplateParameterConfiguration(
                    ParameterId:
                        "state",
                    Name:
                        "State tag")
            ],
            Elements:
            [
                new MimicTemplateElementConfiguration(
                    ElementId:
                        "label",
                    Type:
                        MimicElementType.Text,
                    X:
                        0,
                    Y:
                        0,
                    Width:
                        120,
                    Height:
                        30,
                    Text:
                        "Pump",
                    TagId:
                        null,
                    TagParameterId:
                        null,
                    CommandValue:
                        null),
                new MimicTemplateElementConfiguration(
                    ElementId:
                        "state",
                    Type:
                        MimicElementType.Indicator,
                    X:
                        20,
                    Y:
                        40,
                    Width:
                        80,
                    Height:
                        50,
                    Text:
                        null,
                    TagId:
                        null,
                    TagParameterId:
                        "state",
                    CommandValue:
                        null)
            ]);
    }

    private static async Task<int> ReadSchemaVersionAsync(
        string databasePath)
    {
        await using var connection =
            new SqliteConnection(
                CreateConnectionString(
                    databasePath));
        await connection.OpenAsync();

        await using var command =
            connection.CreateCommand();
        command.CommandText =
            "PRAGMA user_version;";

        return Convert.ToInt32(
            await command.ExecuteScalarAsync());
    }

    private static string CreateConnectionString(
        string databasePath)
    {
        return new SqliteConnectionStringBuilder
        {
            DataSource =
                databasePath,
            Pooling =
                false
        }
        .ToString();
    }

    private sealed class TemporaryDatabase : IDisposable
    {
        private readonly string _directory;

        private TemporaryDatabase(
            string directory,
            string databasePath)
        {
            _directory =
                directory;
            DatabasePath =
                databasePath;
        }

        public string DatabasePath { get; }

        public static TemporaryDatabase Create()
        {
            var directory =
                Path.Combine(
                    Path.GetTempPath(),
                    "dispatcher-tests",
                    Guid.NewGuid().ToString(
                        "N"));

            Directory.CreateDirectory(
                directory);

            return new TemporaryDatabase(
                directory,
                Path.Combine(
                    directory,
                    "dispatcher.db"));
        }

        public void Dispose()
        {
            if (Directory.Exists(
                    _directory))
            {
                Directory.Delete(
                    _directory,
                    recursive:
                        true);
            }
        }
    }
}

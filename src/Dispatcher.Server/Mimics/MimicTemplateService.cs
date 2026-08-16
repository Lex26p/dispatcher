using Dispatcher.Contracts.Mimics;
using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Mimics;

public sealed class MimicTemplateService
{
    private readonly SqliteConfigurationStore _store;
    private readonly MimicConfigurationService _mimicService;
    private readonly SemaphoreSlim _mutationLock =
        new(1, 1);

    public MimicTemplateService(
        SqliteConfigurationStore store,
        MimicConfigurationService mimicService)
    {
        _store =
            store;
        _mimicService =
            mimicService;
    }

    public async Task<IReadOnlyList<MimicTemplateConfiguration>> GetAllAsync(
        CancellationToken cancellationToken)
    {
        return await _store.LoadMimicTemplatesAsync(
            cancellationToken);
    }

    public async Task<MimicTemplateConfiguration?> GetAsync(
        string templateId,
        CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            templateId);

        var templates =
            await _store.LoadMimicTemplatesAsync(
                cancellationToken);

        return templates.FirstOrDefault(template =>
            string.Equals(
                template.TemplateId,
                templateId,
                StringComparison.Ordinal));
    }

    public async Task<MimicTemplateConfiguration> UpsertAsync(
        string templateId,
        MimicTemplateConfiguration template,
        CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            templateId);
        ArgumentNullException.ThrowIfNull(
            template);

        if (!string.Equals(
                templateId,
                template.TemplateId,
                StringComparison.Ordinal))
        {
            throw new InvalidOperationException(
                "TemplateId in URL must match TemplateId in request body.");
        }

        MimicTemplateConfigurationValidator.Validate(
            template);

        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            await _store.UpsertMimicTemplateAsync(
                template,
                cancellationToken);

            return template;
        }
        finally
        {
            _mutationLock.Release();
        }
    }

    public async Task<bool> DeleteAsync(
        string templateId,
        CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            templateId);

        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            return await _store.DeleteMimicTemplateAsync(
                templateId,
                cancellationToken);
        }
        finally
        {
            _mutationLock.Release();
        }
    }

    public async Task<MimicConfiguration> InstantiateAsync(
        string mimicId,
        string templateId,
        InstantiateMimicTemplateRequest request,
        CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            mimicId);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            templateId);
        ArgumentNullException.ThrowIfNull(
            request);
        ArgumentNullException.ThrowIfNull(
            request.TagBindings);

        if (request.X < 0
            || request.Y < 0)
        {
            throw new InvalidOperationException(
                "Template insertion position must be non-negative.");
        }

        var template =
            await GetAsync(
                templateId,
                cancellationToken)
            ?? throw new KeyNotFoundException(
                $"Mimic template '{templateId}' was not found.");

        ValidateBindings(
            template,
            request.TagBindings);

        var elements =
            template.Elements
                .Select(element =>
                    InstantiateElement(
                        element,
                        request))
                .ToArray();

        return await _mimicService.AppendElementsAsync(
                mimicId,
                elements,
                cancellationToken)
            ?? throw new KeyNotFoundException(
                $"Mimic '{mimicId}' was not found.");
    }

    private static void ValidateBindings(
        MimicTemplateConfiguration template,
        IReadOnlyDictionary<string, string> bindings)
    {
        var parameterIds =
            template.Parameters
                .Select(parameter =>
                    parameter.ParameterId)
                .ToHashSet(
                    StringComparer.Ordinal);

        foreach (var key in bindings.Keys)
        {
            if (!parameterIds.Contains(
                    key))
            {
                throw new InvalidOperationException(
                    $"Unknown mimic template parameter '{key}'.");
            }
        }

        foreach (var parameter in template.Parameters)
        {
            if (!bindings.TryGetValue(
                    parameter.ParameterId,
                    out var tagId)
                || string.IsNullOrWhiteSpace(
                    tagId))
            {
                throw new InvalidOperationException(
                    $"Mimic template parameter '{parameter.ParameterId}' requires a TagId binding.");
            }
        }
    }

    private static MimicElementConfiguration InstantiateElement(
        MimicTemplateElementConfiguration element,
        InstantiateMimicTemplateRequest request)
    {
        var x =
            (long)request.X + element.X;
        var y =
            (long)request.Y + element.Y;

        if (x > int.MaxValue
            || y > int.MaxValue)
        {
            throw new InvalidOperationException(
                $"Template element '{element.ElementId}' insertion position exceeds supported bounds.");
        }

        var tagId =
            string.IsNullOrWhiteSpace(
                element.TagParameterId)
                ? element.TagId
                : request.TagBindings[
                    element.TagParameterId!];

        return new MimicElementConfiguration(
            ElementId:
                Guid.NewGuid().ToString(
                    "N"),
            Type:
                element.Type,
            X:
                (int)x,
            Y:
                (int)y,
            Width:
                element.Width,
            Height:
                element.Height,
            Text:
                element.Text,
            TagId:
                tagId,
            CommandValue:
                element.CommandValue);
    }
}

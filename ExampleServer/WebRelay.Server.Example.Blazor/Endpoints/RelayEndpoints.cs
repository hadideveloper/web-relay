using System.Text.Json;
using System.Text.Json.Serialization;
using WebRelay.Server.Example.Blazor.Services;

namespace WebRelay.Server.Example.Blazor.Endpoints;

public static class RelayEndpoints
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.SnakeCaseLower,
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
    };

    public static void MapRelayEndpoints(this WebApplication app)
    {
        // GET endpoint - ESP32 polls this for commands
        app.MapGet("/api/relay", (RelayCommandService relayService) =>
        {
            var command = relayService.GetAndClearPendingCommand();

            if (command == null)
            {
                // Return empty JSON object when no pending commands
                return Results.Content("{}", "application/json");
            }

            var json = JsonSerializer.Serialize(command, JsonOptions);
            return Results.Content(json, "application/json");
        });

        // POST endpoint - ESP32 sends acknowledgments here
        app.MapPost("/api/relay", async (HttpRequest request, RelayCommandService relayService) =>
        {
            using var reader = new StreamReader(request.Body);
            var body = await reader.ReadToEndAsync();

            try
            {
                var ack = JsonSerializer.Deserialize<RelayAck>(body, JsonOptions);
                if (ack?.CommandId != null)
                {
                    relayService.AcknowledgeCommand(ack.CommandId);
                }
            }
            catch
            {
                // Ignore invalid ACK payloads
            }

            return Results.Ok();
        });
    }
}

public class RelayAck
{
    [JsonPropertyName("command_id")]
    public string? CommandId { get; set; }

    [JsonPropertyName("status")]
    public string? Status { get; set; }
}


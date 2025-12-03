namespace WebRelay.Server.Example.Blazor.Services;

/// <summary>
/// Manages relay command state and queuing for ESP32 polling
/// </summary>
public class RelayCommandService
{
    private readonly object _lock = new();
    private RelayCommand? _pendingCommand;
    private readonly Dictionary<string, PendingCommandInfo> _pendingCommands = new();
    
    public event Action? OnStateChanged;

    public bool Relay1State { get; private set; }
    public bool Relay2State { get; private set; }

    /// <summary>
    /// Queue a command to turn relay 1 on or off
    /// </summary>
    public void SetRelay1(bool state)
    {
        lock (_lock)
        {
            // Don't update state optimistically - wait for ESP32 confirmation
            _pendingCommand = new RelayCommand
            {
                CommandId = Guid.NewGuid().ToString("N")[..8],
                Relay1 = new RelayState { State = state ? 1 : 0 },
                Relay2 = null
            };
            
            // Store command info for later ACK processing
            _pendingCommands[_pendingCommand.CommandId] = new PendingCommandInfo
            {
                RelayNumber = 1,
                TargetState = state
            };
        }
        // Don't trigger state change yet - wait for ACK
    }

    /// <summary>
    /// Queue a command to turn relay 2 on or off
    /// </summary>
    public void SetRelay2(bool state)
    {
        lock (_lock)
        {
            // Don't update state optimistically - wait for ESP32 confirmation
            _pendingCommand = new RelayCommand
            {
                CommandId = Guid.NewGuid().ToString("N")[..8],
                Relay1 = null,
                Relay2 = new RelayState { State = state ? 1 : 0 }
            };
            
            // Store command info for later ACK processing
            _pendingCommands[_pendingCommand.CommandId] = new PendingCommandInfo
            {
                RelayNumber = 2,
                TargetState = state
            };
        }
        // Don't trigger state change yet - wait for ACK
    }

    /// <summary>
    /// Get and clear the pending command (called by ESP32 polling endpoint)
    /// </summary>
    public RelayCommand? GetAndClearPendingCommand()
    {
        lock (_lock)
        {
            var command = _pendingCommand;
            _pendingCommand = null;
            return command;
        }
    }

    /// <summary>
    /// Handle acknowledgment from ESP32
    /// </summary>
    public void AcknowledgeCommand(string commandId)
    {
        lock (_lock)
        {
            if (_pendingCommands.TryGetValue(commandId, out var commandInfo))
            {
                // Update relay state based on the acknowledged command
                if (commandInfo.RelayNumber == 1)
                {
                    Relay1State = commandInfo.TargetState;
                }
                else if (commandInfo.RelayNumber == 2)
                {
                    Relay2State = commandInfo.TargetState;
                }
                
                // Remove from pending commands
                _pendingCommands.Remove(commandId);
                
                // Trigger state change event to update UI
                OnStateChanged?.Invoke();
                
                Console.WriteLine($"Command {commandId} acknowledged by ESP32 - Relay {commandInfo.RelayNumber} set to {(commandInfo.TargetState ? "ON" : "OFF")}");
            }
            else
            {
                Console.WriteLine($"Command {commandId} acknowledged but not found in pending commands");
            }
        }
    }
    
    /// <summary>
    /// Information about a pending command waiting for ACK
    /// </summary>
    private class PendingCommandInfo
    {
        public int RelayNumber { get; set; }
        public bool TargetState { get; set; }
    }
}

public class RelayCommand
{
    public string? CommandId { get; set; }
    public RelayState? Relay1 { get; set; }
    public RelayState? Relay2 { get; set; }
}

public class RelayState
{
    public int State { get; set; }
    public int? Duration { get; set; }
}


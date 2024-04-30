namespace GtaV.TwitchPedSpawner.Client.Data;

public class GameState
{
    public string? status { get; init; }
    public EventSub? eventSub { get; init; }
    public SpawnedPed[]? spawnedPeds { get; init; }
}

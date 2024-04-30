namespace GtaV.TwitchPedSpawner.Client.Shared;

public class CustomReward
{
    public required int Id { get; init; }
    public required ERewardType Type { get; init; }
    public bool IsCreated { get; set; }

    public TwitchCustomReward? TwitchModel { get; set; }
}

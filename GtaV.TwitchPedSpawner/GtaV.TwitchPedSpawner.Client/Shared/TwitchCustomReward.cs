namespace GtaV.TwitchPedSpawner.Client.Shared;

public class TwitchCustomReward
{
    public required string id { get; init; }

    public string? broadcaster_id { get; init; }
    public string? broadcaster_login { get; init; }
    public string? broadcaster_name { get; init; }

    public Dictionary<string, string>? image { get; init; }
    public Dictionary<string, string>? default_image { get; init; }

    public string? background_color { get; set; }

    public string? title { get; set; }
    public int cost { get; set; }
    public bool is_enabled { get; set; }
    public bool is_user_input_required { get; set; }
    public string? prompt { get; set; }

    public bool is_paused { get; init; }
    public bool is_in_stock { get; init; }
    public bool should_redemptions_skip_request_queue { get; init; }

    public TwitchCustomRewardMaxPerStreamSetting? max_per_stream_setting { get; init; }
    public TwitchCustomRewardMaxPerUserPerStreamSetting? max_per_user_per_stream_setting { get; init; }
    public TwitchCustomRewardGlobalCooldownSetting? global_cooldown_setting { get; init; }
}

// 💀
public record TwitchCustomRewardMaxPerStreamSetting(bool is_enabled, int max_per_stream);
public record TwitchCustomRewardMaxPerUserPerStreamSetting(bool is_enabled, int max_per_user_per_stream);
public record TwitchCustomRewardGlobalCooldownSetting(bool is_enabled, int global_cooldown_seconds);

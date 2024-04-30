namespace GtaV.TwitchPedSpawner.Controllers;

using GtaV.TwitchPedSpawner.ApiClient;
using GtaV.TwitchPedSpawner.Client.Shared;
using GtaV.TwitchPedSpawner.Data;
using GtaV.TwitchPedSpawner.Entities;
using GtaV.TwitchPedSpawner.Services;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

[ApiController, Route("[controller]")]
public class RewardController(JwtService _jwtService, TwitchApiClient _twitchApi, MySqlContext _db) : Controller
{
    [HttpGet("[action]")]
    public async Task<IActionResult> Sync()
    {
        var user = await _jwtService.GetUser(Request.Headers.Authorization.ToString());
        if (user is null)
            return StatusCode(StatusCodes.Status401Unauthorized);

        var localRewards = await _db.Rewards.Where(x => x.UserId == user.Id).ToArrayAsync();

        var rewardTypes = new Dictionary<ERewardType, List<RewardEntity>>();
        foreach (var localReward in localRewards)
        {
            if (rewardTypes.ContainsKey(localReward.Type))
                rewardTypes[localReward.Type].Add(localReward);
            else
                rewardTypes[localReward.Type] = new() { localReward };
        }

        // Remove duplicate rewards in database
        foreach ((var _, var list) in rewardTypes)
            if (1 < list.Count)
                for (int i = 1; i < list.Count; i++)
                    _db.Rewards.Remove(list[i]);

        // Create local rewards of missing types
        foreach (var type in Enum.GetValues<ERewardType>())
        {
            if (rewardTypes.ContainsKey(type))
                continue;

            var localReward = new RewardEntity
            {
                UserId = user.Id,
                Type = type,
                IsCreated = false,
                TwitchId = null,
                Title = type switch
                {
                    ERewardType.PedReplacement => "NPC Spawner: Wave hello",
                    ERewardType.Paparazzi => "NPC Spawner: Paparazzi",
                    ERewardType.Companion => "NPC Spawner: Companion",
                    _ => "Spawn an NPC named after you in game"
                },
                Cost = type switch
                {
                    ERewardType.PedReplacement => 50_000,
                    ERewardType.Paparazzi => 150_000,
                    ERewardType.Companion => 500_000,
                    _ => 999_999_999
                },
                BackgroundColor = type switch
                {
                    ERewardType.PedReplacement => "#99efa7",
                    ERewardType.Paparazzi => "#39b6d5",
                    ERewardType.Companion => "#fea345",
                    _ => "#ffffff"
                },
                IsUserInputRequired = false,
                Prompt = ""
            };

            _db.Rewards.Add(localReward);
        }

        await _db.SaveChangesAsync();

        var remoteRewards = await _twitchApi.GetCustomRewardsList(user);
        if (remoteRewards is null)
            return StatusCode(StatusCodes.Status500InternalServerError);
        if (!remoteRewards.IsSuccessStatusCode)
            return StatusCode(remoteRewards.StatusCode);

        // Sync local and remote rewards
        foreach (var localReward in localRewards)
        {
            if (localReward.TwitchId is null && localReward.IsCreated)
                localReward.IsCreated = false;
            else if (localReward.IsCreated)
            {
                var remoteReward = remoteRewards.data.FirstOrDefault(x => x.id == localReward.TwitchId);
                if (remoteReward is null)
                {
                    var remoteRewardRes = await _twitchApi.CreateCustomReward(user, new()
                    {
                        title = localReward.Title,
                        cost = localReward.Cost,
                        is_user_input_required = localReward.IsUserInputRequired,
                        prompt = localReward.Prompt,
                        background_color = localReward.BackgroundColor,
                        is_enabled = true,
                        should_redemptions_skip_request_queue = false
                    });
                    if (remoteRewardRes is null || !remoteRewardRes.IsSuccessStatusCode || remoteRewardRes.data.Length == 0)
                        continue;
                    localReward.TwitchId = remoteRewardRes.data[0].id;
                }
                else
                {
                    localReward.Title = remoteReward.title ?? string.Empty;
                    localReward.Cost = remoteReward.cost;
                    localReward.IsUserInputRequired = remoteReward.is_user_input_required;
                    localReward.Prompt = remoteReward.prompt;
                    localReward.BackgroundColor = remoteReward.background_color;
                }
            }
            else if (localReward.TwitchId is not null && remoteRewards.data.Any(x => x.id == localReward.TwitchId))
                await _twitchApi.DeleteCustomReward(user, localReward.TwitchId);
        }

        // Remove remote rewards that is not in local database
        foreach (var remoteReward in remoteRewards.data)
            if (!localRewards.Any(x => x.TwitchId == remoteReward.id))
                await _twitchApi.DeleteCustomReward(user, remoteReward.id);

        await _db.SaveChangesAsync();

        return NoContent();
    }

    [HttpGet("[action]")]
    public async Task<IActionResult> List()
    {
        var user = await _jwtService.GetUser(Request.Headers.Authorization.ToString());
        if (user is null)
            return StatusCode(StatusCodes.Status401Unauthorized);

        var localRewards = await _db.Rewards.Where(x => x.UserId == user.Id).ToArrayAsync();

        var remoteRewards = await _twitchApi.GetCustomRewardsList(user);
        if (remoteRewards is null)
            return StatusCode(StatusCodes.Status500InternalServerError);
        if (!remoteRewards.IsSuccessStatusCode)
            return StatusCode(remoteRewards.StatusCode);

        var rewards = new List<CustomReward>();
        foreach (var localReward in localRewards)
        {
            var remoteReward = remoteRewards.data.FirstOrDefault(x => x.id == localReward.TwitchId);
            if (localReward.IsCreated && remoteReward is null)
            {
                var remoteRewardRes = await _twitchApi.CreateCustomReward(user, new()
                {
                    title = localReward.Title,
                    cost = localReward.Cost,
                    is_user_input_required = localReward.IsUserInputRequired,
                    prompt = localReward.Prompt,
                    background_color = localReward.BackgroundColor,
                    is_enabled = true,
                    should_redemptions_skip_request_queue = false
                });
                if (remoteRewardRes is not null && remoteRewardRes.IsSuccessStatusCode && 0 < remoteRewardRes.data.Length)
                {
                    remoteReward = remoteRewardRes.data[0];
                    localReward.TwitchId = remoteReward.id;
                    await _db.SaveChangesAsync();
                }
            }

            var reward = new CustomReward
            {
                Id = localReward.Id,
                Type = localReward.Type,
                IsCreated = localReward.IsCreated,
                TwitchModel = remoteReward
            };
            rewards.Add(reward);
        }

        return Json(rewards);
    }

    [HttpPost("[action]/{rewardId:int}")]
    public async Task<IActionResult> Update(int rewardId, [FromBody] CustomRewardReq req)
    {
        var user = await _jwtService.GetUser(Request.Headers.Authorization.ToString());
        if (user is null)
            return StatusCode(StatusCodes.Status401Unauthorized);

        var localReward = await _db.Rewards.FirstOrDefaultAsync(x => x.Id == rewardId);

        if (localReward is null)
            return StatusCode(StatusCodes.Status404NotFound);
        if (localReward.UserId != user.Id)
            return StatusCode(StatusCodes.Status403Forbidden);
        if (!localReward.IsCreated)
            return StatusCode(StatusCodes.Status400BadRequest);
        if (localReward.TwitchId is null)
            return StatusCode(StatusCodes.Status500InternalServerError);

        var remoteRewardRes = await _twitchApi.UpdateCustomReward(user, localReward.TwitchId, req);
        if (remoteRewardRes is null)
            return StatusCode(StatusCodes.Status500InternalServerError);
        if (!remoteRewardRes.IsSuccessStatusCode)
            return StatusCode(remoteRewardRes.StatusCode);
        if (remoteRewardRes.data.Length == 0)
            return StatusCode(StatusCodes.Status500InternalServerError);

        var remoteReward = remoteRewardRes.data[0];

        localReward.IsCreated = true;
        localReward.TwitchId = remoteReward.id;
        localReward.Title = remoteReward.title ?? string.Empty;
        localReward.Cost = remoteReward.cost;
        localReward.IsUserInputRequired = remoteReward.is_user_input_required;
        localReward.Prompt = remoteReward.prompt;
        localReward.BackgroundColor = remoteReward.background_color;

        await _db.SaveChangesAsync();

        var reward = new CustomReward
        {
            Id = localReward.Id,
            IsCreated = localReward.IsCreated,
            Type = localReward.Type,
            TwitchModel = remoteReward
        };

        return Json(reward);
    }

    [HttpPost("[action]")]
    public async Task<IActionResult> Enable([FromForm] int id)
    {
        var user = await _jwtService.GetUser(Request.Headers.Authorization.ToString());
        if (user is null)
            return StatusCode(StatusCodes.Status401Unauthorized);

        var localReward = await _db.Rewards.FirstOrDefaultAsync(x => x.Id == id);
        if (localReward is null)
            return StatusCode(StatusCodes.Status404NotFound);
        if (localReward.UserId != user.Id)
            return StatusCode(StatusCodes.Status403Forbidden);

        if (localReward.IsCreated)
            return StatusCode(StatusCodes.Status409Conflict);

        var remoteRewardRes = await _twitchApi.CreateCustomReward(user, new()
        {
            title = localReward.Title,
            cost = localReward.Cost,
            is_user_input_required = localReward.IsUserInputRequired,
            prompt = localReward.Prompt,
            background_color = localReward.BackgroundColor,
            is_enabled = true,
            should_redemptions_skip_request_queue = false
        });
        if (remoteRewardRes is null)
            return StatusCode(StatusCodes.Status500InternalServerError);
        if (!remoteRewardRes.IsSuccessStatusCode)
            return StatusCode(remoteRewardRes.StatusCode);
        if (remoteRewardRes.data.Length == 0)
            return StatusCode(StatusCodes.Status500InternalServerError);

        localReward.IsCreated = true;
        localReward.TwitchId = remoteRewardRes.data[0].id;
        await _db.SaveChangesAsync();

        var reward = new CustomReward
        {
            Id = localReward.Id,
            IsCreated = localReward.IsCreated,
            Type = localReward.Type,
            TwitchModel = remoteRewardRes.data[0]
        };

        return Json(reward);
    }

    [HttpPost("[action]")]
    public async Task<IActionResult> Disable([FromForm] int id)
    {
        var user = await _jwtService.GetUser(Request.Headers.Authorization.ToString());
        if (user is null)
            return StatusCode(StatusCodes.Status401Unauthorized);

        var localReward = await _db.Rewards.FirstOrDefaultAsync(x => x.Id == id);
        if (localReward is null)
            return StatusCode(StatusCodes.Status404NotFound);
        if (localReward.UserId != user.Id)
            return StatusCode(StatusCodes.Status403Forbidden);

        if (!localReward.IsCreated)
            return StatusCode(StatusCodes.Status409Conflict);

        if (localReward.TwitchId is not null)
            await _twitchApi.DeleteCustomReward(user, localReward.TwitchId);

        localReward.IsCreated = false;
        localReward.TwitchId = null;
        await _db.SaveChangesAsync();

        var reward = new CustomReward
        {
            Id = localReward.Id,
            IsCreated = localReward.IsCreated,
            Type = localReward.Type,
            TwitchModel = null
        };

        return Json(reward);
    }

    [HttpGet("[action]")]
    public async Task<IActionResult> Redemption()
    {
        var user = await _jwtService.GetUser(Request.Headers.Authorization.ToString());
        if (user is null)
            return StatusCode(StatusCodes.Status401Unauthorized);

        var localRewards = await _db.Rewards.Where(x => x.UserId == user.Id && x.IsCreated).ToArrayAsync();
        var redemptions = new List<Redemption>();
        foreach (var localReward in localRewards)
        {
            if (localReward.TwitchId is null)
                continue;

            var res = await _twitchApi.GetFirstUnfulfilledRedemption(user, localReward.TwitchId);
            if (res is not null && res.IsSuccessStatusCode && 0 < res.data.Length)
            {
                res.data[0].reward_type = localReward.Type;
                redemptions.Add(res.data[0]);
            }
        }

        if (redemptions.Count == 0)
            return StatusCode(StatusCodes.Status404NotFound);

        return Json(redemptions);
    }

    [HttpPost("[action]")]
    public async Task<IActionResult> Fulfill([FromForm] string redemptionId, [FromForm] int rewardType)
    {
        var user = await _jwtService.GetUser(Request.Headers.Authorization.ToString());
        if (user is null)
            return StatusCode(StatusCodes.Status401Unauthorized);

        var rt = (ERewardType)rewardType;
        var rewards = await _db.Rewards.Where(x => x.UserId == user.Id && x.Type == rt && x.IsCreated).ToArrayAsync();
        foreach (var reward in rewards)
            if (reward.TwitchId is not null)
                await _twitchApi.UpdateRedemptionStatus(user, reward.TwitchId, redemptionId, TwitchApiClient.ERedemptionStatus.FULFILLED);

        return StatusCode(StatusCodes.Status204NoContent);
    }

    [HttpPost("[action]")]
    public async Task<IActionResult> Cancel([FromForm] string redemptionId, [FromForm] int rewardType)
    {
        var user = await _jwtService.GetUser(Request.Headers.Authorization.ToString());
        if (user is null)
            return StatusCode(StatusCodes.Status401Unauthorized);

        var rt = (ERewardType)rewardType;
        var rewards = await _db.Rewards.Where(x => x.UserId == user.Id && x.Type == rt && x.IsCreated).ToArrayAsync();
        foreach (var reward in rewards)
            if (reward.TwitchId is not null)
                await _twitchApi.UpdateRedemptionStatus(user, reward.TwitchId, redemptionId, TwitchApiClient.ERedemptionStatus.CANCELED);

        return StatusCode(StatusCodes.Status204NoContent);
    }
}

namespace GtaV.TwitchPedSpawner.Services;

using GtaV.TwitchPedSpawner.Entities;
using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.Infrastructure;
using Microsoft.EntityFrameworkCore.Storage;

public class MySqlContext : DbContext
{
    public required DbSet<UserEntity> Users { get; init; }
    public required DbSet<RewardEntity> Rewards { get; init; }

    private IConfiguration _config;

    public MySqlContext(IConfiguration config)
    {
        _config = config;

        Database.EnsureCreated();
        try
        {
            Database.Migrate();
        }
        catch { }
    }

    protected override void OnConfiguring(DbContextOptionsBuilder optionsBuilder)
    {
        optionsBuilder.UseMySql(
            $"server={_config["MySql:Host"]};" +
            $"user={_config["MySql:User"]};" +
            $"password={_config["MySql:Password"]};" +
            $"database={_config["MySql:Database"]}",
            new MySqlServerVersion(_config["MySql:Version"])
        );
    }
}
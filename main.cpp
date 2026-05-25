#include "raylib.h"
#include <iostream>
#include <vector>
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <filesystem>
#include <map>
#include <cmath>
#include <algorithm>

struct Animator {
    std::string current_animation;
    float frame_timer;
    int current_frame;
    float fps;

    void Play(std::string name) {
        if (current_animation != name) {
            current_animation = name;
            current_frame = 0;
            frame_timer = 0;
        }
    }

    void Update(float delta_time, int total_frames) {
        if (total_frames <= 0) return;
        frame_timer += delta_time;
        if (frame_timer >= (1.0f / fps)) {
            frame_timer = 0;
            current_frame = (current_frame + 1) % total_frames;
        }
    }
};

struct Player {
    Texture2D texture;
    Vector2 position;
    Vector2 scaled_width_height;
    Animator animator;
    float speed;
    float health;
    float max_health;
    float rotation;
    float scale;
    bool invincible;
    int points;
    int melee_damage;
    bool is_alive;
};

struct Gun {
    int damage;
    int ammo_in_magazine;
    int magazine_size;
    int max_all_ammo;
    int max_current_ammo;
    float reload_time;
    float bullet_speed;
    bool is_unlocked;
};

struct Enemy {
    Animator animator;
    Vector2 position;
    Vector2 direction;
    Texture2D texture;
    float speed;
    int health;
    int damage;
    float rotation;
    float scale;
    int enemy_state = 0;
    float attack_animation_timer = 2;
    bool is_walking = true;
    bool is_attacking = false;
    float attacking_timer;
};

struct Bullet {
    Vector2 position;
    Vector2 direction;
    float speed;
};

void bullet_controller(std::vector<Bullet>& bullets, Player &player) {
    float despawnMargin = 100.0f;
    float boundX = (GetScreenWidth() / 2.0f) + despawnMargin;
    float boundY = (GetScreenHeight() / 2.0f) + despawnMargin;

    for (size_t i = 0; i < bullets.size(); i++) {
        bullets[i].position.x += bullets[i].direction.x * bullets[i].speed * GetFrameTime();
        bullets[i].position.y += bullets[i].direction.y * bullets[i].speed * GetFrameTime();

        DrawCircleV(bullets[i].position, 4, ORANGE);

        if (bullets[i].position.x < player.position.x - boundX || bullets[i].position.x > player.position.x + boundX ||
            bullets[i].position.y < player.position.y - boundY || bullets[i].position.y > player.position.y + boundY) {
            bullets.erase(bullets.begin() + i);
            i--;
        }
    }
}

void enemy_controller(std::vector<Enemy>& enemies, Player &player, std::map<std::string, std::vector<Texture2D>> &animations, int &enemies_to_defeat) {
    float extraBuffer = 100.0f; 
    float boundaryX = (GetScreenWidth() / 2.0f) + extraBuffer;
    float boundaryY = (GetScreenHeight() / 2.0f) + extraBuffer;

    for (size_t i = 0; i < enemies.size(); i++) {
        if (enemies[i].is_walking){
            enemies[i].position.x += enemies[i].direction.x * enemies[i].speed * GetFrameTime();
            enemies[i].position.y += enemies[i].direction.y * enemies[i].speed * GetFrameTime();
        }
        
        float dx = player.position.x - enemies[i].position.x;
        float dy = player.position.y - enemies[i].position.y;
        float radians = atan2f(dy, dx);
        
        enemies[i].rotation = radians * (180.0f / PI);

        if (enemies[i].is_walking) {
            enemies[i].animator.Play("enemy_walk");
        }
        else if (enemies[i].is_attacking){
            enemies[i].attacking_timer -= GetFrameTime();
            enemies[i].animator.Play("enemy_attack");
        }

        if (enemies[i].attacking_timer <= 0){
            enemies[i].is_attacking = false;
            enemies[i].is_walking = true;
        }

        std::string animName = enemies[i].animator.current_animation;
        std::vector<Texture2D>& frames = animations[animName];

        if (frames.empty()) continue;

        if (enemies[i].animator.current_frame >= frames.size()) {
            enemies[i].animator.current_frame = 0;
        }

        Texture2D currentFrameTex = frames[enemies[i].animator.current_frame];
        
        Rectangle source = { 0.0f, 0.0f, (float)currentFrameTex.width, (float)currentFrameTex.height };
        float destW = (float)currentFrameTex.width * enemies[i].scale;
        float destH = (float)currentFrameTex.height * enemies[i].scale;
        Rectangle dest = { enemies[i].position.x, enemies[i].position.y, destW, destH };
        Vector2 origin = { destW / 2.0f, destH / 2.0f };

        enemies[i].animator.Update(GetFrameTime(), frames.size());
        DrawTexturePro(currentFrameTex, source, dest, origin, enemies[i].rotation, WHITE);

        if (enemies[i].position.y > player.position.y + boundaryY) {
            enemies[i].position.y = player.position.y - boundaryY;
        }
        else if (enemies[i].position.y < player.position.y - boundaryY) {
            enemies[i].position.y = player.position.y + boundaryY;
        }

        if (enemies[i].position.x > player.position.x + boundaryX) {
            enemies[i].position.x = player.position.x - boundaryX;
        }
        else if (enemies[i].position.x < player.position.x - boundaryX) {
            enemies[i].position.x = player.position.x + boundaryX;
        }

       
        Vector2 diff = Vector2Subtract(player.position, enemies[i].position);
        enemies[i].direction = Vector2Normalize(diff);
        

        if (enemies[i].health <= 0) {
            player.points += 150; 
            enemies_to_defeat--;
            enemies.erase(enemies.begin() + i);
            i--;
        }
    }
}

void collision_controller(std::vector<Bullet>& bullets, std::vector<Enemy>& enemies, Player& player, Gun &gun, bool is_melee, bool &has_melee_hit) {
    for (size_t j = 0; j < enemies.size(); j++) {
        if (CheckCollisionCircles(player.position, 25, enemies[j].position, 30) && !enemies[j].is_attacking){
            if (!player.invincible){
                player.health -= enemies[j].damage;
                player.invincible = true;
                if (player.health <= 0){
                    player.is_alive = false;
                }
            }
            enemies[j].is_attacking = true;
            enemies[j].is_walking = false;
            enemies[j].attacking_timer = 0.8f;
        }

        if (is_melee && !has_melee_hit) {
            if (CheckCollisionCircles(player.position, 50, enemies[j].position, 40)){
                enemies[j].health -= player.melee_damage;
                has_melee_hit = true;
            }
        }

        for (size_t i = 0; i < bullets.size(); i++) {
            if (CheckCollisionCircles(bullets[i].position, 5, enemies[j].position, 30)) {
                enemies[j].health -= gun.damage;
                bullets.erase(bullets.begin() + i);
                i--;
                break; 
            }
        }
    }
}

void spawn_enemy(float &spawn_timer, std::vector<Enemy> &enemies, Player &player, int &enemies_to_spawn, int &wave){
    float spawnRate = std::max(0.4f, 1.0f - (wave * 0.06f)); 
    if (spawn_timer >= spawnRate && enemies_to_spawn > 0) { 
        Enemy newEnemy;
        
        if (GetRandomValue(0, 1) == 1){ 
            if (GetRandomValue(0, 1) == 1){
                newEnemy.position.y = player.position.y - GetScreenHeight() / 2 - 50;
            }
            else{
                newEnemy.position.y = player.position.y + GetScreenHeight() / 2 + 50;
            }
            newEnemy.position.x = player.position.x + GetRandomValue(-GetScreenWidth() / 2, GetScreenWidth() / 2);
        }
        else{
            if (GetRandomValue(0, 1) == 1){
                newEnemy.position.x = player.position.x - GetScreenWidth() / 2 - 50;
            } 
            else{
                newEnemy.position.x = player.position.x + GetScreenWidth() / 2 + 50;
            }
            newEnemy.position.y = player.position.y + GetRandomValue(-GetScreenHeight() / 2, GetScreenHeight() / 2);
        }     
        
        newEnemy.speed = 160.0f + (wave * 6.0f);   
        newEnemy.health = 10 + (wave * 3);         
        newEnemy.damage = 5 + (wave * 1.5f);       
        newEnemy.rotation = 0;
        newEnemy.scale = 0.6f;
        newEnemy.direction = Vector2Normalize(Vector2Subtract(player.position, newEnemy.position));
        newEnemy.animator.fps = 20;
        
        enemies.push_back(newEnemy);
        spawn_timer = 0.0f;
        enemies_to_spawn--;
    }
}

void load_animation(std::map<std::string, std::vector<Texture2D>>& lib, std::string key, std::string path) {
    if (!std::filesystem::exists(path)) return;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        Texture2D tex = LoadTexture(entry.path().string().c_str());
        lib[key].push_back(tex);
    }
}

void wave_controller(int &enemies_to_defeat, int &wave, float &wave_timer, bool &is_buy_menu_open, int &enemies_to_spawn){
    if (enemies_to_defeat <= 0 || wave_timer <= 0){
        wave++;
        is_buy_menu_open = true;
        wave_timer = 30.0f + (wave * 2.0f);
        enemies_to_defeat = 8 + (wave * 2);
        enemies_to_spawn = 8 + (wave * 2);
    }
    wave_timer -= GetFrameTime();
}

void player_bounds(Player &player) {
    if (player.position.x >= 6700) {
        player.position.x = 6700;
    }
    else if (player.position.x <= 970) {
        player.position.x = 970;
    }

    if (player.position.y >= 2450) {
        player.position.y = 2450;
    }
    else if (player.position.y <= 550) {
        player.position.y = 550;
    }
}

void clamp_camera_to_bounds(Camera2D &camera, Vector2 player_pos) {
    camera.target = player_pos;

    float halfScreenWidth = GetScreenWidth() / 2.0f;
    float halfScreenHeight = GetScreenHeight() / 2.0f;

    float mapLeft = 970.0f;
    float mapRight = 6700.0f;
    float mapTop = 550.0f;
    float mapBottom = 2450.0f;

    camera.target.x = std::clamp(camera.target.x, mapLeft + halfScreenWidth, mapRight - halfScreenWidth);
    camera.target.y = std::clamp(camera.target.y, mapTop + halfScreenHeight, mapBottom - halfScreenHeight);
}

int main() {
    InitWindow(GetMonitorWidth(0), GetMonitorHeight(0), "Raylib Game");
    SetWindowState(2);
    InitAudioDevice();
    SetTargetFPS(120);

    Player player;
    player.position = { 960 + float(GetScreenWidth()) / 2, 540 + float(GetScreenHeight() / 2)};
    player.speed = 180;
    player.max_health = 40;
    player.health = player.max_health;
    player.scale = 0.525f;
    player.invincible = false;
    player.animator.fps = 25;
    player.points = 0;
    player.melee_damage = 8;
    player.is_alive = true;

    int hp_level = 1;
    int dmg_level = 1;
    int speed_level = 1;
    int hp_refill_level = 1;

    Gun rifle;
    rifle.bullet_speed = 2200;
    rifle.ammo_in_magazine = 30;
    rifle.damage = 15;
    rifle.magazine_size = 30;
    rifle.max_all_ammo = 450; 
    rifle.max_current_ammo = 120;
    rifle.reload_time = 1.8f; 
    rifle.is_unlocked = false;

    Gun handgun;
    handgun.bullet_speed = 1800;
    handgun.ammo_in_magazine = 10;
    handgun.damage = 7;
    handgun.magazine_size = 10;
    handgun.max_all_ammo = 250; 
    handgun.max_current_ammo = 80;
    handgun.reload_time = 1.2f; 
    handgun.is_unlocked = true;
    
    Gun *current_gun = &handgun;
    int current_gun_state = 0;

    std::map<std::string, std::vector<Texture2D>> animations;
    load_animation(animations, "handgun_idle", "handgun/idle");
    load_animation(animations, "handgun_reload", "handgun/reload");
    load_animation(animations, "handgun_walk", "handgun/move");
    load_animation(animations, "handgun_shoot", "handgun/shoot");
    load_animation(animations, "handgun_melee", "handgun/meleeattack");
    
    load_animation(animations, "rifle_idle", "rifle/idle");
    load_animation(animations, "rifle_reload", "rifle/reload");
    load_animation(animations, "rifle_walk", "rifle/move");
    load_animation(animations, "rifle_shoot", "rifle/shoot");
    load_animation(animations, "rifle_melee", "rifle/meleeattack");

    load_animation(animations, "enemy_walk", "enemy/walk");
    load_animation(animations, "enemy_attack", "enemy/attack");

    float spawn_timer = 0.0f;
    float reload_timer = current_gun->reload_time;
    float invincible_timer = 0.6f;
    Texture2D background_texture = LoadTexture("background.png");

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.target = player.position;
    camera.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    
    Texture2D crosshair_texture = LoadTexture("crosshair.png");

    bool is_reloading = false;
    Vector2 direction = { 0, 0 };
    bool is_moving = false;
    bool is_shooting = false;
    float shoot_anim_timer = 0.0f;
    const float SHOOT_DURATION = 0.12f;

    float wave_timer = 30.0f; 
    bool is_buy_menu_open = false;
    int wave = 1;
    int enemies_to_defeat = 8;
    int enemies_to_spawn = 8;

    bool is_melee = false;
    bool has_melee_hit = false;
    bool is_paused = false;

    Sound zombie_grunts[3];
    zombie_grunts[0] = LoadSound("zombie-8.wav");
    zombie_grunts[1] = LoadSound("zombie-9.wav");
    zombie_grunts[2] = LoadSound("zombie-12.wav");

    float zombie_grunt_timer = (float)GetRandomValue(5, 15);
    Sound gun_shot = LoadSound("shot.mp3");
    Music music = LoadMusicStream("music.mp3");
    PlayMusicStream(music);

    SetMasterVolume(0.5f);
    while (!WindowShouldClose()) {
        
        float delta_time = GetFrameTime();
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        UpdateMusicStream(music);
        float dx = mouseWorldPos.x - player.position.x;
        float dy = mouseWorldPos.y - player.position.y;
        float radians = atan2f(dy, dx);

        if (IsKeyPressed(KEY_P)) {
            is_paused = !is_paused;
            if (is_paused) {
                ShowCursor();
            } else {
                HideCursor();
            }
        }
        
        if (!is_buy_menu_open && player.is_alive && !is_paused){
            spawn_timer += delta_time;
            HideCursor();
            direction = { 0, 0 };
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) direction.y -= 1.0f;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) direction.y += 1.0f;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) direction.x += 1.0f;
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) direction.x -= 1.0f;

            if (!is_reloading){
                if (IsKeyPressed(KEY_ONE) || GetMouseWheelMove() == 1){
                    if (rifle.is_unlocked){
                        current_gun = &rifle;
                        current_gun_state = 1;
                    }
                }
                if (IsKeyPressed(KEY_TWO) || GetMouseWheelMove() == -1){
                    current_gun = &handgun;
                    current_gun_state = 0;
                }
            }
            
            if (Vector2Length(direction) > 0) {
                direction = Vector2Normalize(direction);
                is_moving = true;
            } else {
                is_moving = false;
            }

            player.position.x += direction.x * player.speed * delta_time;
            player.position.y += direction.y * player.speed * delta_time;

            if ((IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) && current_gun->ammo_in_magazine > 0 && !is_reloading) {
                Bullet newBullet;
                float muzzle_forward_offset = 40.0f;
                float muzzle_side_offset = 15.0f;

                Vector2 muzzle_pos = {
                    player.position.x + cosf(radians) * muzzle_forward_offset - sinf(radians) * muzzle_side_offset,
                    player.position.y + sinf(radians) * muzzle_forward_offset + cosf(radians) * muzzle_side_offset
                };
                newBullet.position = muzzle_pos;
                newBullet.speed = current_gun->bullet_speed;
                newBullet.direction = Vector2Normalize(Vector2Subtract(mouseWorldPos, newBullet.position));
                bullets.push_back(newBullet);
                
                current_gun->ammo_in_magazine--;
                is_shooting = true;
                shoot_anim_timer = SHOOT_DURATION;
                PlaySound(gun_shot);
            }

            if (IsKeyPressed(KEY_R) && current_gun->ammo_in_magazine < current_gun->magazine_size && current_gun->max_current_ammo > 0 && !is_reloading){
                reload_timer = current_gun->reload_time;
                is_reloading = true;
                is_moving = false;
            }
            
            if (IsKeyPressed(KEY_F) && !is_reloading){
                is_melee = true;
                has_melee_hit = false; 
                player.animator.Play(current_gun_state == 0 ? "handgun_melee" : "rifle_melee");
            }
        }

        player_bounds(player);
        clamp_camera_to_bounds(camera, player.position);

        if (shoot_anim_timer > 0) {
            shoot_anim_timer -= delta_time;
            is_shooting = true;
        } else {
            is_shooting = false;
        }

        if (is_melee) {
            std::string current_anim = player.animator.current_animation;
            if (animations.find(current_anim) != animations.end() && !animations[current_anim].empty()) {
                int total_melee_frames = animations[current_anim].size();
                if (player.animator.current_frame >= total_melee_frames - 1) {
                    is_melee = false;
                    has_melee_hit = false; // FIXED: Safely reset state flags for the next activation call
                }
            } else {
                is_melee = false; 
            }
        }

        if (zombie_grunt_timer > 0){
            zombie_grunt_timer -= delta_time;
        }
        else{
            zombie_grunt_timer = GetRandomValue(5, 15);
            int rand_idx = GetRandomValue(0, 2);
            PlaySound(zombie_grunts[rand_idx]);
        }

        if (is_reloading){
            reload_timer -= delta_time;
            if (reload_timer <= 0){
                is_reloading = false;
                int needed = current_gun->magazine_size - current_gun->ammo_in_magazine;
                int to_add = (current_gun->max_current_ammo >= needed) ? needed : current_gun->max_current_ammo;
                current_gun->ammo_in_magazine += to_add;
                current_gun->max_current_ammo -= to_add;
            }
        }

        if (player.invincible){
            invincible_timer -= delta_time;
            if (invincible_timer <= 0){
                player.invincible = false;
                invincible_timer = 0.6f;
            }
        } 

        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        BeginMode2D(camera);
        DrawTexture(background_texture, 0, 0, WHITE);
        
        player.rotation = radians * (180.0f / PI);

        if (is_reloading) {
            if (current_gun_state == 0) player.animator.Play("handgun_reload");
            else if (current_gun_state == 1) player.animator.Play("rifle_reload");
        }
        else if (is_melee) {
            if (current_gun_state == 0) player.animator.Play("handgun_melee");
            else if (current_gun_state == 1) player.animator.Play("rifle_melee");
        } 
        else if (is_shooting) {
            if (current_gun_state == 0) player.animator.Play("handgun_shoot");
            else if (current_gun_state == 1) player.animator.Play("rifle_shoot");
        } 
        else if (is_moving) {
            if (current_gun_state == 0) player.animator.Play("handgun_walk");
            else if (current_gun_state == 1) player.animator.Play("rifle_walk");
        } 
        else {
            if (current_gun_state == 0) player.animator.Play("handgun_idle");
            else if (current_gun_state == 1) player.animator.Play("rifle_idle");
        }

        std::string player_anim = player.animator.current_animation;
        if (animations.find(player_anim) != animations.end() && !animations[player_anim].empty()) {
            std::vector<Texture2D>& player_frames = animations[player_anim];
            Texture2D player_texture = player_frames[player.animator.current_frame % player_frames.size()];
            Rectangle player_source = { 0.0f, 0.0f, (float)player_texture.width, (float)player_texture.height };
            float player_destination_width = (float)player_texture.width * player.scale;
            float player_destination_height = (float)player_texture.height * player.scale;
            Rectangle player_dest = { player.position.x, player.position.y, player_destination_width, player_destination_height};
            Vector2 player_origin = { player_destination_width / 2.0f, player_destination_height / 2.0f };

            player.animator.Update(delta_time, player_frames.size());
            DrawTexturePro(player_texture, player_source, player_dest, player_origin, player.rotation, player.invincible ? RED : WHITE);
        } else {
            DrawCircleV(player.position, 20, BLUE); 
        }
        
        if (!is_buy_menu_open && player.is_alive && !is_paused){
            collision_controller(bullets, enemies, player, *current_gun, is_melee, has_melee_hit);
            enemy_controller(enemies, player, animations, enemies_to_defeat);
            bullet_controller(bullets, player);
            wave_controller(enemies_to_defeat, wave, wave_timer, is_buy_menu_open, enemies_to_spawn);
            spawn_enemy(spawn_timer, enemies, player, enemies_to_spawn, wave);
        }
        EndMode2D();
        
        Vector2 screenPos = GetWorldToScreen2D({ player.position.x, player.position.y - 40 }, camera);
        if (player.health > player.max_health) player.health = player.max_health;
        
        if (!is_buy_menu_open && player.is_alive && !is_paused) {
            DrawTextureEx(crosshair_texture, Vector2{GetMousePosition().x - 25, GetMousePosition().y - 25}, 0, 0.05f, WHITE);
        }
        
        DrawRectangle(screenPos.x - 50, screenPos.y - 10, 100, 8, DARKGRAY);
        DrawRectangle(screenPos.x - 50, screenPos.y - 10, (player.health / player.max_health) * 100, 8, RED);

        DrawText(TextFormat("TIMER: %.1f", std::max(0.0f, wave_timer)), GetScreenWidth() / 2 - 50, 10, 20, WHITE);
        DrawText(TextFormat("WAVE: %i", wave), GetScreenWidth() / 2 - 50, 31, 20, WHITE);
        DrawText(TextFormat("POINTS: %i", player.points), 20, GetScreenHeight() - 40, 24, WHITE);

        if (!is_reloading) {
            DrawText(TextFormat("AMMO: %i / %i", current_gun->ammo_in_magazine, current_gun->max_current_ammo), GetScreenWidth() - 200, GetScreenHeight() - 40, 22, WHITE);
        } else {
            DrawText("RELOADING...", GetScreenWidth() - 200, GetScreenHeight() - 40, 22, WHITE);
            DrawRectangle(screenPos.x - 25, screenPos.y - 25, ((current_gun->reload_time - reload_timer) / current_gun->reload_time) * 50, 6, BLUE);
        }

        if (is_buy_menu_open) {
            ShowCursor();
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.6f));
            
            float winW = 750, winH = 500;
            float x = (GetScreenWidth() - winW) / 2;
            float y = (GetScreenHeight() - winH) / 2;

            GuiWindowBox((Rectangle){ x, y, winW, winH }, TextFormat("WAVE %i COMPLETED! ARMORY UPGRADES", wave - 1));

            int hp_cost = 400 * hp_level;
            int dmg_cost = 600 * dmg_level;
            int speed_cost = 500 * speed_level;
            int ammo_cost = 300;
            int hp_refill_cost = 550 * hp_refill_level;

            if (GuiButton((Rectangle){ x + 50, y + 80, 300, 50 }, TextFormat("Max HP +20 (Cost: %i)", hp_cost))) {
                if (player.points >= hp_cost) {
                    player.max_health += 20;
                    player.health = player.max_health;
                    player.points -= hp_cost;
                    hp_level++;
                }
            }

            if (GuiButton((Rectangle){ x + 400, y + 80, 300, 50 }, TextFormat("Weapon Damage +3 (Cost: %i)", dmg_cost))) {
                if (player.points >= dmg_cost) {
                    handgun.damage += 3;
                    rifle.damage += 3;
                    player.melee_damage += 4;
                    player.points -= dmg_cost;
                    dmg_level++;
                }
            }

            if (GuiButton((Rectangle){ x + 50, y + 160, 300, 50 }, TextFormat("Refill All Ammo Reserves (Cost: %i)", ammo_cost))) {
                if (player.points >= ammo_cost) {
                    handgun.max_current_ammo = handgun.max_all_ammo;
                    rifle.max_current_ammo = rifle.max_all_ammo;
                    player.points -= ammo_cost;
                }
            }

            if (GuiButton((Rectangle){ x + 400, y + 160, 300, 50 }, TextFormat("Movement Speed +15 (Cost: %i)", speed_cost))) {
                if (player.points >= speed_cost) {
                    player.speed += 15;
                    player.points -= speed_cost;
                    speed_level++;
                }
            }

            if (GuiButton((Rectangle){ x + 225, y + 235, 300, 50 }, TextFormat("Refill HP (Cost: %i)", hp_refill_cost))) {
                if (player.points >= hp_refill_cost) {
                    player.health = player.max_health;
                    player.points -= hp_refill_cost;
                    hp_refill_level++;
                }
            }

            if (!rifle.is_unlocked) {
                if (GuiButton((Rectangle){ x + 150, y + 300, 450, 60 }, "UNLOCK ASSAULT RIFLE (Cost: 6000)")) {
                    if (player.points >= 6000) {
                        rifle.is_unlocked = true;
                        player.points -= 6000;
                    }
                }
            } else {
                GuiLabel((Rectangle){ x + 240, y + 310, 400, 40 }, "ASSAULT RIFLE UNLOCKED - Swap with key '1' and '2'");
            }

            if (GuiButton((Rectangle){ x + 275, y + 400, 200, 45 }, "START NEXT WAVE")) {
                is_buy_menu_open = false;
                spawn_timer = 0.0f;
            }
        }

        else if (!player.is_alive) {
            ShowCursor();
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.8f));
            
            float winW = 500, winH = 300;
            float x = (GetScreenWidth() - winW) / 2;
            float y = (GetScreenHeight() - winH) / 2;

            GuiWindowBox((Rectangle){ x, y, winW, winH }, "GAME OVER");
            DrawText(TextFormat("You survived until Wave: %i", wave), x + 100, y + 80, 20, WHITE);

            if (GuiButton((Rectangle){ x + 150, y + 140, 200, 40 }, "RESTART")) {
                player.position = { 960 + float(GetScreenWidth()) / 2, 540 + float(GetScreenHeight() / 2)};
                player.speed = 180;
                player.max_health = 40;
                player.health = player.max_health;
                player.points = 0;
                player.melee_damage = 8;
                player.is_alive = true;
                player.invincible = false;

                hp_level = 1;
                dmg_level = 1;
                speed_level = 1;
                hp_refill_level = 1;

                rifle.max_all_ammo = 450;
                rifle.ammo_in_magazine = 30;
                rifle.damage = 15;
                rifle.max_current_ammo = 120;
                rifle.is_unlocked = false;

                handgun.max_all_ammo = 250;
                handgun.ammo_in_magazine = 10;
                handgun.damage = 7;
                handgun.max_current_ammo = 80;

                current_gun = &handgun;
                current_gun_state = 0;

                enemies.clear();
                bullets.clear();
                
                spawn_timer = 0.0f;
                wave_timer = 30.0f; 
                wave = 1;
                enemies_to_defeat = 8;
                enemies_to_spawn = 8;
                
                is_reloading = false;
                is_moving = false;
                is_melee = false;
                is_buy_menu_open = false;
                is_paused = false;
                camera.target = player.position;
            }

            if (GuiButton((Rectangle){ x + 150, y + 200, 200, 40 }, "EXIT")) {
                break;
            }
        }
        
        else if (is_paused){
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.8f));
            
            float x = GetScreenWidth() / 2 - 50;
            float y = GetScreenHeight() / 2;
            DrawText("PAUSED", x, y, 25, WHITE);
        }
        
        EndDrawing();
    }

    UnloadTexture(background_texture);
    UnloadTexture(crosshair_texture);
    UnloadMusicStream(music);
    UnloadSound(gun_shot);
    
    for(int i = 0; i < 3; i++) {
        UnloadSound(zombie_grunts[i]);
    }
    
    for (auto const& [name, frames] : animations) {
        for (Texture2D tex : frames) UnloadTexture(tex);
    }
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
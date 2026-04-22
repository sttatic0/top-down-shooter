#include "raylib.h"
#include <iostream>
#include <vector>
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <filesystem>
#include <map>

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
    float armor;
    float health_regen;
    float rotation;
    float scale;
    bool invincible;
    int points;
    int melee_damage;
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

struct Exp_orb {
    Vector2 position;
    //Vector2 direction;    //TODO
    int value;
};




void bullet_controller(std::vector<Bullet>& bullets, Player player) {
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


void drop_exp(std::vector<Exp_orb>& exp_orbs, float enemy_x, float enemy_y) {
    Exp_orb exp_orb;
    exp_orb.position.x = enemy_x;
    exp_orb.position.y = enemy_y;
    exp_orb.value = 130;
    exp_orbs.push_back(exp_orb);
}

void enemy_controller(std::vector<Enemy>& enemies, Player &player, Texture2D enemyTex, std::vector<Exp_orb>& exp_orbs, std::map<std::string, std::vector<Texture2D>> &animations, int &enemies_to_defeat) {

    float extraBuffer = 100.0f; // teleport slightly offscreen
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
            
        
        bool needsRecalc = true;

        

        if (enemies[i].position.y > player.position.y + boundaryY) {
            enemies[i].position.y = player.position.y - boundaryY;
            needsRecalc = true;
        }
        else if (enemies[i].position.y < player.position.y - boundaryY) {
            enemies[i].position.y = player.position.y + boundaryY;
            needsRecalc = true;
        }

        if (enemies[i].position.x > player.position.x + boundaryX) {
            enemies[i].position.x = player.position.x - boundaryX;
            needsRecalc = true;
        }
        else if (enemies[i].position.x < player.position.x - boundaryX) {
            enemies[i].position.x = player.position.x + boundaryX;
            needsRecalc = true;
        }

       // std::cout << "Timer: " << timer << '\n';

        if (needsRecalc) {
            Vector2 diff = Vector2Subtract(player.position, enemies[i].position);
            enemies[i].direction = Vector2Normalize(diff);
        }

        if (enemies[i].health <= 0) {
            //drop_exp(exp_orbs, enemies[i].position.x, enemies[i].position.y);
            player.points += 100;
            enemies_to_defeat--;
            enemies.erase(enemies.begin() + i);
            i--;
        }
    }
}

void collision_controller(std::vector<Bullet>& bullets, std::vector<Enemy>& enemies, Player& player, std::vector<Exp_orb>& exp_orbs, Texture2D enemyTex, Texture2D orbTex, Gun &gun, bool is_melee) {

    for (size_t j = 0; j < enemies.size(); j++) {
   
        if (CheckCollisionCircles(player.position, 5, enemies[j].position, 40) && !enemies[j].is_attacking){
            if (!player.invincible){
                player.health -= enemies[j].damage;
                player.invincible = true;
            }
            enemies[j].is_attacking = true;
            enemies[j].is_walking = false;
            enemies[j].attacking_timer = 1;
        }
        if (CheckCollisionCircles(player.position, 40, enemies[j].position, 40) && is_melee){
            enemies[j].health -= player.melee_damage;
        }

        for (size_t i = 0; i < bullets.size(); i++) {
            if (CheckCollisionCircles(bullets[i].position, 5, enemies[j].position, 40)) {
                enemies[j].health -= gun.damage;
                bullets.erase(bullets.begin() + i);
                i--;
            }
        }
    }

    for (size_t k = 0; k < exp_orbs.size(); k++) {
        DrawTextureEx(orbTex, exp_orbs[k].position, 0, 0.2f, WHITE);

        if (CheckCollisionCircles(player.position, 40, exp_orbs[k].position, 5)) {
            exp_orbs.erase(exp_orbs.begin() + k);
            k--;
        }
    }
}

void spawn_enemy(float &spawn_timer, std::vector<Enemy> &enemies, Player player, int &enemies_to_spawn){
    if (spawn_timer >= 1 && enemies_to_spawn > 0) { //&& GetTime() < 4) {
        Enemy newEnemy;
        
        
        if (GetRandomValue(0, 1) == 1){ //Top/Bottom or Left/Right spawn


            if (GetRandomValue(0, 1) == 1){//Top or bottom spawn
                
                newEnemy.position.y = player.position.y - GetScreenHeight() / 2 - 50;

            }
            else{
                newEnemy.position.y = player.position.y + GetScreenHeight() / 2 + 50;
            }
            newEnemy.position.x = player.position.x + GetRandomValue(-GetScreenWidth() / 2, GetScreenWidth() / 2);
        }
        else{
            if (GetRandomValue(0, 1) == 1){//Left or right spawn
                newEnemy.position.x = player.position.x - GetScreenWidth() / 2 - 50;
            } 
            else{
                newEnemy.position.x = player.position.x + GetScreenWidth() / 2 + 50;

            }
            newEnemy.position.y = player.position.y + GetRandomValue(-GetScreenHeight() / 2, GetScreenHeight() / 2);
        }     
        
        
        newEnemy.speed = 100;
        newEnemy.health = 10;
        newEnemy.damage = 5;
        newEnemy.rotation = 0;
        newEnemy.scale = 0.3f;
        newEnemy.direction = Vector2Normalize(Vector2Subtract(player.position, newEnemy.position));

        newEnemy.animator.fps = 12;
        enemies.push_back(newEnemy);
        spawn_timer -= 1;

        enemies_to_spawn--;
    }
}

void load_animation(std::map<std::string, std::vector<Texture2D>>& lib, std::string key, std::string path) {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        Texture2D tex = LoadTexture(entry.path().string().c_str());
        lib[key].push_back(tex);
    }
}


void debug_colliders(std::vector<Enemy> &enemies, Texture2D enemyTex, Player player){
    for (size_t i = 0; i < enemies.size(); i++) {
        DrawCircleLinesV(enemies[i].position, 40, RED);
    }
    DrawCircleLinesV(player.position, 40, RED);
}


void wave_controller(int &enemies_to_defeat, int &wave, float &wave_timer, bool &is_buy_menu_open, int &enemies_to_spawn){
    if (enemies_to_defeat <= 0 || wave_timer <= 0){
        wave++;
        is_buy_menu_open = true;
        wave_timer = 30;
        enemies_to_defeat = 5;
        enemies_to_spawn = 5;
    }

    wave_timer -= GetFrameTime();
}

int main() {
    int screen_width = GetMonitorWidth(GetCurrentMonitor()); 
    
    int screen_height = GetMonitorHeight(GetCurrentMonitor());
    //SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(1920, 1080, "Raylib Game");
    SetTargetFPS(120);

    Player player;
    Texture2D enemyTexture = LoadTexture("enemy.png");
    
    Texture2D orbTexture = LoadTexture("exp_orb.png");

    player.position.x = 500;
    player.position.y = 500;
    player.speed = 150;
    player.max_health = 30;
    player.health = player.max_health;
    player.scale = 0.3f;
    player.scaled_width_height = {player.texture.width * player.scale, player.texture.height * player.scale};
    player.invincible = false;
    player.animator.fps = 15;
    player.points = 0;
    player.melee_damage = 4;


    Gun rifle;
    rifle.bullet_speed = 2500;
    rifle.ammo_in_magazine = 30;
    rifle.damage = 15;
    rifle.magazine_size = 30;
    rifle.max_all_ammo = 90;
    rifle.max_current_ammo = 30;
    rifle.reload_time = 2.0f; 
    rifle.is_unlocked = false;

    Gun handgun;
    handgun.bullet_speed = 2500;
    handgun.ammo_in_magazine = 10;
    handgun.damage = 5;
    handgun.magazine_size = 10;
    handgun.max_all_ammo = 40;
    handgun.max_current_ammo = 10;
    handgun.reload_time = 1.5f; 
    handgun.is_unlocked = true;
    
    Gun *current_gun = &handgun;
    int current_gun_state = 0;

    std::map<std::string, std::vector<Texture2D>> animations;
    load_animation(animations, "rifle_idle", "rifle/idle");
    load_animation(animations, "rifle_walk", "rifle/move");
    load_animation(animations, "rifle_shoot", "rifle/shoot");
    load_animation(animations, "rifle_reload", "rifle/reload");
    load_animation(animations, "rifle_melee", "rifle/meleeattack");
    load_animation(animations, "handgun_idle", "handgun/idle");
    load_animation(animations, "handgun_reload", "handgun/reload");
    load_animation(animations, "handgun_walk", "handgun/move");
    load_animation(animations, "handgun_shoot", "handgun/shoot");
    load_animation(animations, "handgun_melee", "handgun/meleeattack");

    load_animation(animations, "enemy_walk", "enemy/walk");
    load_animation(animations, "enemy_attack", "enemy/attack");
    //load_animation(animations, "zombie_walk", "assets/enemies/zombie/walk");
    //load_animation(animations, "handgun_shoot", "assets/handgun/shoot");

    float spawn_timer = 0.0f;
    float reload_timer = current_gun->reload_time;
    float invincible_timer = 2.0f;
    Image bgImg = LoadImage("bullet.png");
    if (bgImg.data != NULL) ImageResize(&bgImg, 100, 100);
    Texture2D background_texture = LoadTextureFromImage(bgImg);
    UnloadImage(bgImg);

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.target = player.position;
    camera.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    std::vector<Exp_orb> exp_orbs;
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    
    
    Texture2D crosshair_texture = LoadTexture("crosshair.png");

    bool is_reloading = false;
    //misc
    Vector2 direction = { 0, 0 };
    /*for(int i = 0; i < sizeof(animator_table_test); i++){
        std::cout << animator_table_test[i] << '\n';
    }*/
    bool is_moving = false;
    
    bool is_shooting;

    float shoot_anim_timer = 0.0f;
    const float SHOOT_DURATION = 0.15f;

    //Wave shit
    float wave_timer = 30; 
    bool is_buy_menu_open = false;
    int wave = 1;
    int enemies_to_defeat = 5;
    int enemies_to_spawn = 5;

    bool is_melee = false;
    float melee_timer;
    while (!WindowShouldClose()) {

        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

        float dx = mouseWorldPos.x - player.position.x;
        float dy = mouseWorldPos.y - player.position.y;

        float radians = atan2f(dy, dx);
        float delta_time = GetFrameTime();
        spawn_timer += delta_time;

        // Player Movement
        if (!is_buy_menu_open){
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
                //std::cout << "Is moving" << '\n';
            }
            else{
                is_moving = false;
            }


            player.position.x += direction.x * player.speed * delta_time;
            player.position.y += direction.y * player.speed * delta_time;

            camera.target = { (player.position.x), (player.position.y) };

            
            // Shooting
            if ((IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) && current_gun->ammo_in_magazine > 0 && !is_reloading) {
                Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
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
            }
            if (IsKeyPressed(KEY_R) && current_gun->ammo_in_magazine < current_gun->magazine_size && current_gun->max_current_ammo > 0 && !is_reloading){
                reload_timer = current_gun->reload_time;
                is_reloading = true;
                is_moving = false;
            }
            if (IsKeyPressed(KEY_F) && !is_reloading){
                is_melee = true;
                melee_timer = 1.0f;
            }
            // Spawning
            
        }

        
        if (shoot_anim_timer > 0) {
            shoot_anim_timer -= delta_time;
            is_shooting = true;
        } else {
            is_shooting = false;
        }

        if (melee_timer > 0) {
            melee_timer -= delta_time;
            is_melee = true;
        } else {
            is_melee = false;
        }
        
        //Reloading
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

        /*while (player.exp >= player.level_threshold){
            player.level++;
            player.exp -= player.level_threshold;
            player.level_threshold *= 1.66f;
        }*/
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        if (player.invincible == true){
            if (invincible_timer <= 0){
                player.invincible = false;
                invincible_timer = 2.0f;
            }
            invincible_timer -= delta_time;
        }
        BeginMode2D(camera);
        DrawTexture(background_texture, 0, 0, WHITE);

        debug_colliders(enemies, enemyTexture, player);
        
        player.rotation = radians * (180.0f / PI);

   
        if (is_reloading) {
            
            if (current_gun_state == 0) 
                player.animator.Play("handgun_reload");
            else if (current_gun_state == 1) 
                player.animator.Play("rifle_reload");
        }
        else if (is_melee) {
            if (current_gun_state == 0)
                player.animator.Play("handgun_melee");
            else if (current_gun_state == 1) 
                player.animator.Play("rifle_melee");
        } 
        else if (is_shooting) {
           
            if (current_gun_state == 0)
                player.animator.Play("handgun_shoot");
            else if (current_gun_state == 1) 
                player.animator.Play("rifle_shoot");
        } 
        else if (is_moving) {
            
            if (current_gun_state == 0) 
                player.animator.Play("handgun_walk");
            else if (current_gun_state == 1) 
                player.animator.Play("rifle_walk");
        } 
        else {
            
            if (current_gun_state == 0) 
                player.animator.Play("handgun_idle");
            else if (current_gun_state == 1) 
                player.animator.Play("rifle_idle");
        }

        std::string player_anim = player.animator.current_animation;

        
        std::vector<Texture2D>& player_frames = animations[player_anim];

        
            
        Texture2D player_texture = player_frames[player.animator.current_frame];

        Rectangle player_source = { 0.0f, 0.0f, (float)player_texture.width, (float)player_texture.height };

        float player_destination_width = (float)player_texture.width * player.scale;
        float player_destination_height = (float)player_texture.height * player.scale;
        Rectangle player_dest = { player.position.x, player.position.y, player_destination_width, player_destination_height };

        Vector2 player_origin = { player_destination_width / 2.0f, player_destination_height / 2.0f };


        player.animator.Update(delta_time, player_frames.size());
        DrawTexturePro(player_texture, player_source, player_dest, player_origin, player.rotation, WHITE);
        
        

        
        if (!is_buy_menu_open){
            collision_controller(bullets, enemies, player, exp_orbs, enemyTexture, orbTexture, *current_gun, is_melee);
            enemy_controller(enemies, player, enemyTexture, exp_orbs, animations, enemies_to_defeat);
            bullet_controller(bullets, player);
            wave_controller(enemies_to_defeat, wave, wave_timer, is_buy_menu_open, enemies_to_spawn);
            spawn_enemy(spawn_timer, enemies, player, enemies_to_spawn);
        }
        EndMode2D();
        std::cout << spawn_timer << '\n';

        
        Vector2 screenPos = GetWorldToScreen2D({ player.position.x, player.position.y - 20 }, camera);
        
        //DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.4f));
        
        if (player.health >= player.max_health) 
            player.health = player.max_health;
        
        if (!is_buy_menu_open)
            DrawTextureEx(crosshair_texture, Vector2{GetMousePosition().x + screenPos.x - GetScreenWidth() / 2 - 25, GetMousePosition().y + screenPos.y - GetScreenHeight() / 2 - 5}, 0, 0.05f, WHITE);
        
        //HP bar
        DrawRectangle(screenPos.x - 55, screenPos.y - 30, (player.health / player.max_health) * 100, 7, RED);
        //GuiProgressBar((Rectangle){ screenPos.x - 25, screenPos.y, player.max_health, 5 }, NULL, NULL, &player.health, 0, player.max_health); 
        
        DrawText(TextFormat("TIMER: %.1f", wave_timer), GetScreenWidth() / 2 - 20, 10, 20, DARKGRAY);
        DrawText(TextFormat("WAVE: %i", wave), GetScreenWidth() / 2 - 20, 30, 20, DARKGRAY);
        DrawText(TextFormat("POINTS: %i", player.points), 10, GetScreenHeight() - 30, 20, DARKGRAY);

        if (!is_reloading)
            DrawText(TextFormat("%i / %i", current_gun->ammo_in_magazine, current_gun->max_current_ammo), GetScreenWidth() - 150, GetScreenHeight() - 50, 20, DARKGRAY);
        else{
            DrawText(TextFormat("Reloading..."), GetScreenWidth() - 150, GetScreenHeight() - 50, 20, DARKGRAY);
            DrawRectangle(screenPos.x - 25, screenPos.y - 50, (reload_timer / current_gun->reload_time) * 50, 5, BLUE);
        }


        if (is_buy_menu_open) {
            ShowCursor();
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.5f));
            
            float winW = 700, winH = 400;
            float x = (GetScreenWidth() - winW) / 2;
            float y = (GetScreenHeight() - winH) / 2;

            GuiWindowBox((Rectangle){ x, y, winW, winH }, TextFormat("WAVE %i COMPLETED! SHOP", wave - 1));

            // Option 1: Max HP
            if (GuiButton((Rectangle){ x + 50, y + 60, 280, 40 }, "Increase Max HP (Cost: 500)") && player.points >= 500) {
                player.max_health += 20;
                player.health = player.max_health;
                player.points -= 500;
            }

            // Option 2: Damage
            if (GuiButton((Rectangle){ x + 370, y + 60, 280, 40 }, "Increase Weapon Damage (Cost: 1000)") && player.points >= 1000) {
                current_gun->damage += 5;
                player.points -= 1000;
            }

            // Option 3: Refill Ammo
            if (GuiButton((Rectangle){ x + 50, y + 120, 280, 40 }, "Full Ammo Refill (Cost: 300)") && player.points >= 300) {
                current_gun->max_current_ammo = current_gun->max_all_ammo;
                player.points -= 300;
            }

            // Option 4: Speed
            if (GuiButton((Rectangle){ x + 370, y + 120, 280, 40 }, "Increase Movement Speed (Cost: 800)") && player.points >= 800) {
                player.speed += 20;
                player.points -= 800;
            }

            // Option 5: THE RIFLE
            if (!rifle.is_unlocked) {
                if (GuiButton((Rectangle){ x + 50, y + 200, 600, 60 }, "UNLOCK ASSAULT RIFLE (Cost: 3000)") && player.points >= 3000) {
                    rifle.is_unlocked = true;
                    player.points -= 3000;
                }
            } else {
                GuiLabel((Rectangle){ x + 250, y + 210, 200, 40 }, "RIFLE UNLOCKED - Press '2' to use");
            }

            // Exit Button
            if (GuiButton((Rectangle){ x + 250, y + 320, 200, 40 }, "START NEXT WAVE")) {
                is_buy_menu_open = false;
                spawn_timer = 0;
            }
        }
        EndDrawing();
    }
   
    // Cleanup
    UnloadTexture(enemyTexture);
    UnloadTexture(orbTexture);
    UnloadTexture(background_texture);
    UnloadTexture(crosshair_texture);
    for (auto const& [name, frames] : animations) {
        for (Texture2D tex : frames) UnloadTexture(tex);
    }
    CloseWindow();

    return 0;
}
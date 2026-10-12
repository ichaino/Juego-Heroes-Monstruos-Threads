#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

typedef struct{
    int width;
    int height;
} Grid;

typedef struct {
    int X;
    int Y;
} Coordenadas;

typedef struct{
    int HP;
    int ATTACK_DAMAGE;
    int ATTACK_RANGE;
    int START_X, START_Y;
    Coordenadas* LISTA_MOVIMIENTOS;
    int CANT_MOVIMIENTOS;
    int id;
} Heroe;

typedef struct{
    int HP;
    int ATTACK_DAMAGE;
    int ATTACK_RANGE;
    int VISION_RANGE;
    int START_X, START_Y;
    bool ALERT;
} Monster;

typedef struct {
    Heroe* heroe;
    Monster** monsters;
    int cantidad_M;
    int CANT_MOVIMIENTOS;
    pthread_mutex_t* mutex_heroe;     
    pthread_mutex_t* mutex_monster; 
    Grid* grid;
} ArgHeroe;

bool validacion_mov(Grid* grid, int x, int y) {
    if (x < 0 || x >= grid->width || y < 0 || y >= grid->height) {
        printf("Movimiento inválido: coordenadas (%d,%d) fuera del grid (%dx%d)\n", x, y, grid->width, grid->height);
        return false;
    }
    return true;
}

void leer_configuracion(const char* filename, Grid* grid, Heroe*** heroes, int* cantidad_heroes, Monster*** monsters, int* cantidad_monstruos) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error al abrir el archivo %s\n", filename);
        exit(1);
    }

    char line[512];
    *cantidad_heroes = 0;
    *cantidad_monstruos = 0;
    
    // contar heroes y leer grid
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "HERO_") && strstr(line, "_HP")) {
            (*cantidad_heroes)++;
        }
        else if (strstr(line, "MONSTER_COUNT")) {
            sscanf(line, "MONSTER_COUNT %d", cantidad_monstruos);
        }
        else if (strstr(line, "GRID_SIZE")) {
            sscanf(line, "GRID_SIZE %d %d", &grid->width, &grid->height);
        }
    }
    
    // si no hay heroes con el formato _1_, busco formato simple
    if (*cantidad_heroes == 0) {
        rewind(file);
        while (fgets(line, sizeof(line), file)) {
            if (strstr(line, "HERO_HP")) {
                *cantidad_heroes = 1;
                break;
            }
        }
    }
    
    // creo arreglos
    *heroes = malloc(*cantidad_heroes * sizeof(Heroe*));
    for (int i = 0; i < *cantidad_heroes; i++) {
        (*heroes)[i] = malloc(sizeof(Heroe));
        (*heroes)[i]->LISTA_MOVIMIENTOS = NULL;
        (*heroes)[i]->CANT_MOVIMIENTOS = 0;
        (*heroes)[i]->id = i + 1;
    }
    
    *monsters = malloc(*cantidad_monstruos * sizeof(Monster*));
    for (int i = 0; i < *cantidad_monstruos; i++) {
        (*monsters)[i] = malloc(sizeof(Monster));
        (*monsters)[i]->ALERT = false;
    }
    
    // volver al inicio para leer datos -> primero cuento heroes y monstruos, y segundo leo los datos especificos
    rewind(file);
    
    // leer los datos de los heroes
    int hero_index = -1;
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "HERO_") && (strstr(line, "_HP") || strstr(line, "_ATTACK") || strstr(line, "_START") || strstr(line, "_PATH"))) {
            
            // n heroes
            char* hero_ptr = strstr(line, "HERO_");
            if (hero_ptr != NULL) {
                hero_ptr += 5;  // avanza mas alla de las 5 letras de "HERO_"
                
                // leo el numero del heroe
                int hero_num;
                if (sscanf(hero_ptr, "%d", &hero_num) == 1) {
                    hero_index = hero_num - 1;  // indice - 1
                } else {
                    // si no se lee el numero, es simple (1 herore) 
                    hero_index = 0;
                }
            }
            
            // verifico si el indice es valido y leo las stats
            if (hero_index >= 0 && hero_index < *cantidad_heroes) {
                if (strstr(line, "_HP")) {
                    sscanf(line, "HERO_%*d_HP %d", &(*heroes)[hero_index]->HP);
                    if ((*heroes)[hero_index]->HP == 0) {
                        sscanf(line, "HERO_HP %d", &(*heroes)[hero_index]->HP);
                    }
                }
                else if (strstr(line, "_ATTACK_DAMAGE")) {
                    sscanf(line, "HERO_%*d_ATTACK_DAMAGE %d", &(*heroes)[hero_index]->ATTACK_DAMAGE);
                    if ((*heroes)[hero_index]->ATTACK_DAMAGE == 0) {
                        sscanf(line, "HERO_ATTACK_DAMAGE %d", &(*heroes)[hero_index]->ATTACK_DAMAGE);
                    }
                }
                else if (strstr(line, "_ATTACK_RANGE")) {
                    sscanf(line, "HERO_%*d_ATTACK_RANGE %d", &(*heroes)[hero_index]->ATTACK_RANGE);
                    if ((*heroes)[hero_index]->ATTACK_RANGE == 0) {
                        sscanf(line, "HERO_ATTACK_RANGE %d", &(*heroes)[hero_index]->ATTACK_RANGE);
                    }
                }
                else if (strstr(line, "_START")) {
                    sscanf(line, "HERO_%*d_START %d %d", &(*heroes)[hero_index]->START_X, &(*heroes)[hero_index]->START_Y);
                    if ((*heroes)[hero_index]->START_X == 0 && (*heroes)[hero_index]->START_Y == 0) {
                        sscanf(line, "HERO_START %d %d", &(*heroes)[hero_index]->START_X, &(*heroes)[hero_index]->START_Y);
                    }
                }
                else if (strstr(line, "_PATH")) {
                    int capacidad = 50;
                    (*heroes)[hero_index]->LISTA_MOVIMIENTOS = malloc(capacidad * sizeof(Coordenadas));
                    int count = 0;
                    
                    char* ptr = line;
                    while (*ptr != '\0') {
                        if (*ptr == '(') { // busco donde empieza la coordenada
                            int x, y;
                            if (sscanf(ptr, "(%d,%d)", &x, &y) == 2) {
                                // si el array supera la capacidad, lo duplico
                                if (count >= capacidad) {
                                    capacidad *= 2;
                                    (*heroes)[hero_index]->LISTA_MOVIMIENTOS = realloc((*heroes)[hero_index]->LISTA_MOVIMIENTOS, capacidad * sizeof(Coordenadas));
                                }
                                // guardo la coordenada
                                (*heroes)[hero_index]->LISTA_MOVIMIENTOS[count].X = x;
                                (*heroes)[hero_index]->LISTA_MOVIMIENTOS[count].Y = y;
                                count++;
                            }
                        }
                        ptr++; // avanzo al siguiente caracter
                    }
                    
                    long pos = ftell(file); // guardo donde estoy en el archivo
                    while (fgets(line, sizeof(line), file)) { // leo la siguiente linea del archivo
                        if (strstr(line, "HERO_") || strstr(line, "MONSTER_")) {
                            fseek(file, pos, SEEK_SET); // si la linea no corresponde al path, retrocedo
                            break;
                        }
                        
                        // misma logica anterior
                        ptr = line;
                        while (*ptr != '\0') {
                            if (*ptr == '(') {
                                int x, y;
                                if (sscanf(ptr, "(%d,%d)", &x, &y) == 2) {
                                    if (count >= capacidad) {
                                        capacidad *= 2;
                                        (*heroes)[hero_index]->LISTA_MOVIMIENTOS = realloc((*heroes)[hero_index]->LISTA_MOVIMIENTOS, capacidad * sizeof(Coordenadas));
                                    }
                                    (*heroes)[hero_index]->LISTA_MOVIMIENTOS[count].X = x;
                                    (*heroes)[hero_index]->LISTA_MOVIMIENTOS[count].Y = y;
                                    count++;
                                }
                            }
                            ptr++;
                        }
                        pos = ftell(file); // guardo la nueva posicion actual
                    }
                    
                    (*heroes)[hero_index]->CANT_MOVIMIENTOS = count; // cantidad de movimientos del heroe
                }
            }
        }
    }
    
    // leer datos de los monstruos
    rewind(file); // vuelvo al inicio del archivo
    int monster_index = -1;
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "MONSTER_")) {
            char* ptr = line;
            while (*ptr != '\0') {
                if (*ptr == '_') { // si se encuentra "_"
                    ptr++; // avanzo al siguiete caracter
                    if (sscanf(ptr, "%d", &monster_index) == 1) {
                        monster_index--; // empiezo en indice 0
                        break;
                    }
                }
                ptr++; // si no encuentro "_" avanzo al siguiente caracter
            }
            
            // leo las stats de cada monstruo
            if (monster_index >= 0 && monster_index < *cantidad_monstruos) {
                if (strstr(line, "_HP")) sscanf(line, "MONSTER_%*d_HP %d", &(*monsters)[monster_index]->HP);
                else if (strstr(line, "_ATTACK_DAMAGE")) sscanf(line, "MONSTER_%*d_ATTACK_DAMAGE %d", &(*monsters)[monster_index]->ATTACK_DAMAGE);
                else if (strstr(line, "_VISION_RANGE")) sscanf(line, "MONSTER_%*d_VISION_RANGE %d", &(*monsters)[monster_index]->VISION_RANGE);
                else if (strstr(line, "_ATTACK_RANGE")) sscanf(line, "MONSTER_%*d_ATTACK_RANGE %d", &(*monsters)[monster_index]->ATTACK_RANGE);
                else if (strstr(line, "_COORDS")) sscanf(line, "MONSTER_%*d_COORDS %d %d", &(*monsters)[monster_index]->START_X, &(*monsters)[monster_index]->START_Y);
            }
        }
    }
    
    fclose(file); // cierro el archivo
    printf("Archivo de configuración leído exitosamente:\n");
    printf("Grid: %dx%d\n", grid->width, grid->height);
    printf("Héroes: %d\n", *cantidad_heroes);
    for (int i = 0; i < *cantidad_heroes; i++) {
        printf("  Héroe %d: HP=%d, DAÑO=%d, RANGO=%d, INICIO=(%d,%d), MOVIMIENTOS=%d\n", 
               i+1, (*heroes)[i]->HP, (*heroes)[i]->ATTACK_DAMAGE, (*heroes)[i]->ATTACK_RANGE, 
               (*heroes)[i]->START_X, (*heroes)[i]->START_Y, (*heroes)[i]->CANT_MOVIMIENTOS);
    }
    printf("Monstruos: %d\n", *cantidad_monstruos);
}

void* funcion_heroe(void* arg){
    ArgHeroe* args = (ArgHeroe*) arg;
    Heroe* heroe = args->heroe;
    int CANT_MOVIMIENTOS = args->CANT_MOVIMIENTOS;
    pthread_mutex_t* mutex_heroe = args->mutex_heroe;
    pthread_mutex_t* mutex_monster = args->mutex_monster;
    Grid* grid = args->grid;
    Monster** all_monster = args->monsters;
    int cantidad_M = args->cantidad_M;

    pthread_mutex_lock(mutex_heroe);
    printf("El heroe %d inicia en la posicion: (%d,%d)\n", heroe->id, heroe->START_X, heroe->START_Y);
    pthread_mutex_unlock(mutex_heroe);

    for(int i = 0; i < CANT_MOVIMIENTOS; i++){
        pthread_mutex_lock(mutex_heroe);
        int nueva_x = heroe->LISTA_MOVIMIENTOS[i].X;
        int nueva_y = heroe->LISTA_MOVIMIENTOS[i].Y;
        if (validacion_mov(grid, nueva_x, nueva_y)) {
            heroe->START_X = nueva_x;
            heroe->START_Y = nueva_y;
            printf("El heroe %d avanzo a la poscion: (%d,%d)\n", heroe->id, heroe->START_X ,heroe->START_Y );
        }
        pthread_mutex_unlock(mutex_heroe);

        bool attack = false;
        do{
            attack = false;
            pthread_mutex_lock(mutex_heroe);
            int h_x = heroe->START_X;
            int h_y = heroe->START_Y;
            int h_attack_range = heroe->ATTACK_RANGE;
            int h_attack_damage = heroe->ATTACK_DAMAGE;
            pthread_mutex_unlock(mutex_heroe);

            for(int j = 0; j < cantidad_M; j++){
                pthread_mutex_lock(mutex_monster);
                int m_hp = all_monster[j]->HP;
                int m_x = all_monster[j]->START_X;
                int m_y = all_monster[j]->START_Y;
                pthread_mutex_unlock(mutex_monster);
                      
                if(m_hp <= 0) continue;
                
                int dist = abs(h_x- m_x) + abs(h_y - m_y);
                if(dist <= h_attack_range && m_hp > 0){
                    pthread_mutex_lock(mutex_monster);
                    all_monster[j]->HP -= h_attack_damage;
                    printf("Heroe %d ataca al M%d -> (-%d)HP Monstruo \n", heroe->id, j, h_attack_damage);
                    
                    if(all_monster[j]->HP > 0){
                        printf("vida monstruo M%d (%d)\n",j, all_monster[j]->HP);
                    }

                    if(all_monster[j]->HP <= 0){
                        all_monster[j]->HP = 0;
                        printf("Monstruo M%d esta muerto\n", j);
                    }
                    int hp_monster = all_monster[j]->HP;
                    pthread_mutex_unlock(mutex_monster);

                    if(hp_monster > 0) attack = true;
                }      
            }
            if(attack) sleep(1);
        }while(attack);
        sleep(1);
    }

    printf("El héroe %d terminó su ruta.\n", heroe->id);
    return NULL;
}

Heroe* encontrar_heroe_mas_cercano(Heroe** heroes, int cantidad_heroes, int monstruo_x, int monstruo_y) {
    Heroe* mas_cercano = heroes[0];
    int distancia_minima = abs(monstruo_x - heroes[0]->START_X) + abs(monstruo_y - heroes[0]->START_Y);
    
    for (int i = 1; i < cantidad_heroes; i++) {
        int distancia = abs(monstruo_x - heroes[i]->START_X) + abs(monstruo_y - heroes[i]->START_Y);
        if (distancia < distancia_minima) {
            distancia_minima = distancia;
            mas_cercano = heroes[i];
        }
    }
    return mas_cercano;
}

typedef struct{
    Monster** monsters;
    int cantidad_M;
    int id_monster;
    Heroe** heroes;
    int cantidad_heroes;
    pthread_mutex_t* mutex_heroe;     
    pthread_mutex_t* mutex_monster; 
    Grid* grid;
} ArgMonster;

void* funcion_monstruo(void* arg){
    ArgMonster* args = (ArgMonster*) arg;
    Monster** all_monster = args->monsters;
    Monster* monster = args->monsters[args->id_monster];
    int cantidad_M = args->cantidad_M;
    int id_monster = args->id_monster;
    pthread_mutex_t* mutex_monster = args->mutex_monster;
    Grid* grid = args->grid;
    Heroe** heroes = args->heroes;
    int cantidad_heroes = args->cantidad_heroes;
    pthread_mutex_t* mutex_heroe = args->mutex_heroe;

    bool GO_ALERT = true;
    bool TURNO_MOV = true;

    while(true){
        pthread_mutex_lock(mutex_heroe);
        Heroe* heroe_objetivo = encontrar_heroe_mas_cercano(heroes, cantidad_heroes, monster->START_X, monster->START_Y);
        int h_hp = heroe_objetivo->HP;
        int h_x = heroe_objetivo->START_X;
        int h_y = heroe_objetivo->START_Y;
        pthread_mutex_unlock(mutex_heroe);

        pthread_mutex_lock(mutex_monster);
        int m_hp = monster->HP;
        bool m_alert = monster->ALERT;
        int m_x = monster->START_X;
        int m_y = monster->START_Y;
        int m_attack_damage = monster->ATTACK_DAMAGE;
        int m_attack_range = monster->ATTACK_RANGE;
        int m_vision_range = monster->VISION_RANGE;
        pthread_mutex_unlock(mutex_monster);

        if(m_hp <= 0 || h_hp <= 0) break;

        if(m_alert){
            if(GO_ALERT){
                pthread_mutex_lock(mutex_monster);
                for(int i = 0; i < cantidad_M; i++){
                    int dist_alert = abs(m_x - all_monster[i]->START_X) + abs(m_y - all_monster[i]->START_Y);
                    if(dist_alert <= m_vision_range) all_monster[i]->ALERT = true;
                }
                pthread_mutex_unlock(mutex_monster);
                GO_ALERT = false;
                sleep(1);
                continue;
            }
            else{
                int dist_attack = abs(m_x- h_x) + abs(m_y - h_y);
                if(dist_attack <= m_attack_range){
                    pthread_mutex_lock(mutex_heroe);
                    if(heroe_objetivo->HP > 0){
                        heroe_objetivo->HP -= m_attack_damage;
                        printf("Monstruo M%d ataca al Héroe %d -> (-%d)HP\n", id_monster, heroe_objetivo->id, m_attack_damage);
                        if(heroe_objetivo->HP <= 0) heroe_objetivo->HP = 0;
                    }
                    printf("vida héroe %d (%d)\n", heroe_objetivo->id, heroe_objetivo->HP);
                    int hp_h = heroe_objetivo->HP;
                    pthread_mutex_unlock(mutex_heroe);
                    if(hp_h <= 0) break;
                }
                else{
                    int dx = abs(m_x- h_x);
                    int dy = abs(m_y - h_y);
                    if(dx > dy) TURNO_MOV = true;
                    else if(dy > dx) TURNO_MOV = false;
                    else TURNO_MOV = !TURNO_MOV;
                    
                    pthread_mutex_lock(mutex_monster);
                    int nueva_x = monster->START_X;
                    int nueva_y = monster->START_Y;
                    
                    if(TURNO_MOV){
                        if(h_x < monster->START_X) nueva_x = monster->START_X - 1;
                        else if(h_x > monster->START_X) nueva_x = monster->START_X + 1;
                    }
                    else{
                        if(h_y < monster->START_Y) nueva_y = monster->START_Y - 1;
                        else if(h_y > monster->START_Y) nueva_y = monster->START_Y + 1;
                    }
                    
                    if (validacion_mov(grid, nueva_x, nueva_y)) {
                        monster->START_X = nueva_x;
                        monster->START_Y = nueva_y;
                        printf("Monstruo M%d se mueve hacia Héroe %d -> (%d,%d)\n", id_monster, heroe_objetivo->id, monster->START_X, monster->START_Y);
                    }
                    pthread_mutex_unlock(mutex_monster);
                }
            }
        }
        else if(!monster->ALERT){
            int dist = abs(m_x- h_x) + abs(m_y - h_y);
            if (dist <= monster->VISION_RANGE){
                pthread_mutex_lock(mutex_monster);
                monster->ALERT = true;
                pthread_mutex_unlock(mutex_monster);
                printf("Monstruo M%d alertado! Ve al Héroe %d en (%d,%d)\n", id_monster, heroe_objetivo->id, h_x, h_y);
            }
        }
        usleep(500000);
    }   
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s <archivo_configuracion>\n", argv[0]);
        return 1;
    }

    Grid grid;
    Heroe** heroes;
    int cantidad_heroes;
    Monster** monsters;
    int cantidad_monstruos;
    
    leer_configuracion(argv[1], &grid, &heroes, &cantidad_heroes, &monsters, &cantidad_monstruos);
    printf("Iniciando simulación con %d héroes y %d monstruos...\n", cantidad_heroes, cantidad_monstruos);

    pthread_mutex_t mutex_heroe = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex_monster = PTHREAD_MUTEX_INITIALIZER;

    pthread_t threads_heroes[cantidad_heroes];
    ArgHeroe args_heroes[cantidad_heroes];

    for (int h = 0; h < cantidad_heroes; h++) {
        args_heroes[h].heroe = heroes[h];
        args_heroes[h].monsters = monsters;
        args_heroes[h].cantidad_M = cantidad_monstruos;
        args_heroes[h].CANT_MOVIMIENTOS = heroes[h]->CANT_MOVIMIENTOS;
        args_heroes[h].mutex_heroe = &mutex_heroe;
        args_heroes[h].mutex_monster = &mutex_monster;
        args_heroes[h].grid = &grid;
        pthread_create(&threads_heroes[h], NULL, funcion_heroe, &args_heroes[h]);
    }

    pthread_t threads_monsters[cantidad_monstruos];
    ArgMonster args_monsters[cantidad_monstruos];

    for (int i = 0; i < cantidad_monstruos; i++) {
        args_monsters[i].monsters = monsters;
        args_monsters[i].cantidad_M = cantidad_monstruos;
        args_monsters[i].id_monster = i;
        args_monsters[i].heroes = heroes;
        args_monsters[i].cantidad_heroes = cantidad_heroes;
        args_monsters[i].mutex_heroe = &mutex_heroe;
        args_monsters[i].mutex_monster = &mutex_monster;
        args_monsters[i].grid = &grid;
        pthread_create(&threads_monsters[i], NULL, funcion_monstruo, &args_monsters[i]);
    }

    for (int h = 0; h < cantidad_heroes; h++) pthread_join(threads_heroes[h], NULL);
    for (int i = 0; i < cantidad_monstruos; i++) pthread_join(threads_monsters[i], NULL);

    for (int h = 0; h < cantidad_heroes; h++) {
        free(heroes[h]->LISTA_MOVIMIENTOS);
        free(heroes[h]);
    }
    free(heroes);
    
    for (int i = 0; i < cantidad_monstruos; i++) free(monsters[i]);
    free(monsters);

    printf("Simulación terminada.\n");
    return 0;
}
#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <random>
#include <iostream>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	virtual bool is_game_over() override {
		return missed >= 5;
	}

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// dolphin:
	static inline Scene::Transform *dolphin = nullptr;
	// dolphin jump animation
	bool is_jumping = false;
	float jump_speed = 40.0f;
	float velocity = 0.0f;
	float MAX_VELOCITY = 5.0f;
	float acceleration = 25.0f;
	float gravity = -10.0f;
	glm::vec3 dolphin_base_rotation_euler;
	glm::vec3 dolphin_curr_rotation_euler;
	float ROTATION_X_MAX_DEG = 60.0f;
	// restrict the frequency of dolphin movement in order to make it more controllable
	float pos_change_period = 0.0f;
	float POS_CHANGE_MIN_PERIOD = 0.2f;
	void update_dolphin(float elapsed);

	// torus:
	float show_next_torus_period;
	float SHOW_NEXT_TORUS_PERIOD_MIN = 3.0f;
	float SHOW_NEXT_TORUS_PERIOD_MAX = 6.0f;
	static inline float JUMP_RANGE_Z_DELTA = 0.5f;
	static inline std::array<float,3> torus_positions_x = {-10.0f, 0.0f, 10.0f};
	static inline std::array<float,2> torus_positions_z = {5.0f, 10.0f};
	static inline float TORUS_HIDE_Y = -40.0f;
	static inline float TORUS_SHOW_Y = 50.0f;
	static inline float START_JUMP_POS = -5.0f;
	static inline float END_JUMP_POS = -12.0f;

	struct Torus {
		float torus_speed = 10.0f;
		bool is_showing = false;
		bool is_jumped_over = false;
		Scene::Transform *transform = nullptr;

		Torus() {}
		Torus(Scene::Transform *transform_) : transform(transform_) {}

		void show() {
			static std::random_device rd; 
			static std::mt19937 gen(rd()); 
			static std::uniform_int_distribution<> dist_x(0, 2);
			static std::uniform_int_distribution<> dist_y(0, 1);

			transform->position.x = torus_positions_x[dist_x(gen)];
			transform->position.z = torus_positions_z[dist_y(gen)];
			transform->position.y = TORUS_SHOW_Y;
			is_showing = true;
			is_jumped_over = false;
		}

		void hide() {
			transform->position.y = TORUS_HIDE_Y;
			is_showing = false;
		}

		void move(float elapsed) {
			transform->position.y -= torus_speed * elapsed;
			if(transform->position.y < TORUS_HIDE_Y) {
				hide();
			}
		}

		bool in_jump_range() {
			bool in_jump_range_y =  transform->position.y <= START_JUMP_POS && transform->position.y >= END_JUMP_POS;
			bool in_jump_position_x = transform->position.x == dolphin->position.x;
			bool in_jump_range_z = transform->position.z - JUMP_RANGE_Z_DELTA <= dolphin->position.z && dolphin->position.z <= transform->position.z + JUMP_RANGE_Z_DELTA;
			return in_jump_range_y && in_jump_position_x && in_jump_range_z;
		}
	};
	std::array<Torus,3> toruses = {};
	uint32_t curr_torus_idx = 0;
	
	void set_next_show_torus_period() {
		static std::random_device rd; 
		static std::mt19937 gen(rd()); 
		static std::uniform_real_distribution<> dist(0.0f, 1.0f);
		show_next_torus_period = SHOW_NEXT_TORUS_PERIOD_MIN + (SHOW_NEXT_TORUS_PERIOD_MAX - SHOW_NEXT_TORUS_PERIOD_MIN) * dist(gen);
	}

	void update_torus(float elapsed);
	
	// camera:
	Scene::Camera *camera = nullptr;

	// plane:
	Scene::Transform *plane = nullptr;
	float PLANE_START_POS_X = 80.0f;
	float PLANE_END_POS_X = -10.0f;
	float plane_move_speed = 4.0f;
	void update_plane(float elapsed);

	// ----- game state -----
	uint32_t score = 0;
	uint32_t missed = 0;
	float timer = 0.0f;
	float speed_up_period = 10.0f;
	float speed_up_factor = 1.2f;
	float speed_up_delta = 0.3f;
	void speed_up(float elapsed);
};

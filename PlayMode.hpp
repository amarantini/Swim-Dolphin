#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <random>

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
	Scene::Transform *dolphin = nullptr;
	// dolphin jump animation
	bool is_jumping = false;
	float jump_speed = 40.0f;
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
	struct Torus {
		float TORUS_HIDE_Y = -30.0f;
		float TORUS_SHOW_Y = 50.0f;
		float START_JUMP_POS = -5.0f;
		float END_JUMP_POS = -12.0f;
		float torus_speed = 10.0f;
		bool is_showing = false;
		bool is_jumped_over = false;
		Scene::Transform *transform = nullptr;

		Torus() {}
		Torus(Scene::Transform *transform_) : transform(transform_) {}

		void show_at(float x) {
			transform->position.x = x;
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
			return transform->position.y <= START_JUMP_POS && transform->position.y >= END_JUMP_POS;
		}
	};
	std::array<Torus,3> toruses = {};
	uint32_t curr_torus_idx = 0;
	std::array<float,3> torus_positions = {-10.0f, 0.0f, 10.0f};

	float get_next_torus_position() {
		static std::random_device rd; 
		static std::mt19937 gen(rd()); 
		static std::uniform_int_distribution<> dist(0, 2);
		return torus_positions[dist(gen)];
	}
	
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

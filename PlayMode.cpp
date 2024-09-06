#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>



GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("dolphin.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("dolphin.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	for (auto &transform : scene.transforms) {
		if (transform.name == "Dolphin") dolphin = &transform;
		else if (transform.name == "Torus1") toruses[0] = Torus(&transform);
		else if (transform.name == "Torus2") toruses[1] = Torus(&transform);
		else if (transform.name == "Torus3") toruses[2] = Torus(&transform);
		else if (transform.name == "Plane") plane = &transform;
	}
	if(dolphin == nullptr) throw std::runtime_error("Dolphin not found.");
	if(toruses[0].transform == nullptr) throw std::runtime_error("Torus1 not found.");
	if(toruses[1].transform == nullptr) throw std::runtime_error("Torus2 not found.");
	if(toruses[2].transform == nullptr) throw std::runtime_error("Torus3 not found.");

	dolphin_base_rotation_euler = glm::eulerAngles(dolphin->rotation) / float(M_PI) * 180.0f;
	dolphin_curr_rotation_euler = dolphin_base_rotation_euler;

	//get pointers to leg for convenience:
	// for (auto &transform : scene.transforms) {
	// 	if (transform.name == "Hip.FL") hip = &transform;
	// 	else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
	// 	else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
	// }
	// if (hip == nullptr) throw std::runtime_error("Hip not found.");
	// if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	// if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");

	// hip_base_rotation = hip->rotation;
	// upper_leg_base_rotation = upper_leg->rotation;
	// lower_leg_base_rotation = lower_leg->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} 
	// else if (evt.type == SDL_MOUSEBUTTONDOWN) {
	// 	if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
	// 		SDL_SetRelativeMouseMode(SDL_TRUE);
	// 		return true;
	// 	}
	// } else if (evt.type == SDL_MOUSEMOTION) {
	// 	if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
	// 		glm::vec2 motion = glm::vec2(
	// 			evt.motion.xrel / float(window_size.y),
	// 			-evt.motion.yrel / float(window_size.y)
	// 		);
	// 		camera->transform->rotation = glm::normalize(
	// 			camera->transform->rotation
	// 			* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
	// 			* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
	// 		);
	// 		return true;
	// 	}
	// }

	return false;
}

void PlayMode::update(float elapsed) {
	// update dolphin
	update_dolphin(elapsed);

	// update toruses
	update_torus(elapsed);

	// update plane
	update_plane(elapsed);

	// update speed
	speed_up(elapsed);

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Score: " + std::to_string(score) + ", Missed: " + std::to_string(missed),
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Score: " + std::to_string(score)  + ", Missed: " + std::to_string(missed),
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}

void PlayMode::update_dolphin(float elapsed) {
	// dolphin jumps animation:
	if(is_jumping) {
		dolphin_curr_rotation_euler.x -= elapsed * jump_speed;
		dolphin->rotation = glm::quat(dolphin_curr_rotation_euler * float(M_PI) / 180.0f);
		if(dolphin_curr_rotation_euler.x <= dolphin_base_rotation_euler.x - ROTATION_X_MAX_DEG) {
			is_jumping = false;
			dolphin_curr_rotation_euler = dolphin_base_rotation_euler;
			dolphin->rotation = glm::quat(dolphin_base_rotation_euler * float(M_PI) / 180.0f);
		}
	}

	// 	move dolphin
	float dolphin_position_x = dolphin->position.x;
	if (left.pressed && !right.pressed && pos_change_period >= POS_CHANGE_MIN_PERIOD) {
		dolphin_position_x = std::max(-10.0f, dolphin_position_x - 10.0f);
		pos_change_period = 0.0f;
	} else if (!left.pressed && right.pressed && pos_change_period >= POS_CHANGE_MIN_PERIOD) {
		dolphin_position_x = std::min(10.0f, dolphin_position_x + 10.0f);
		pos_change_period = 0.0f;
	} else {
		pos_change_period += elapsed;
	}
	dolphin->position.x = dolphin_position_x;
}

void PlayMode::update_torus(float elapsed) {
	if(show_next_torus_period >= 0.0f) {
		show_next_torus_period -= elapsed;
	} else {
		// show next torus in a random position
		curr_torus_idx = (curr_torus_idx + 1) % 3;
		if(!toruses[curr_torus_idx].is_showing) {
			toruses[curr_torus_idx].show_at(get_next_torus_position());
			set_next_show_torus_period();
		}
	}
	
	for(auto &torus : toruses) {
		if (torus.is_showing) {
			torus.move(elapsed);
			if(!is_jumping && torus.in_jump_range() && torus.transform->position.x == dolphin->position.x) {
				// dolphin start jumping
				std::cout<<"Dolphin jumps!"<<std::endl;
				is_jumping = true;
				torus.is_jumped_over = true;
				++score;
			}
			if(!torus.is_jumped_over && torus.transform->position.y < torus.END_JUMP_POS) {
				torus.is_jumped_over = true;
				++missed;
			}
		}
	}
}

void PlayMode::update_plane(float elapsed) {
	plane->position.y -= plane_move_speed * elapsed;
	if(plane->position.y < PLANE_END_POS_X) {
		plane->position.y = PLANE_START_POS_X;
	}
}

void PlayMode::speed_up(float elapsed) {
	timer += elapsed;
	if(timer >= speed_up_period) {
		timer = 0.0f;
		for(auto &torus : toruses) {
			torus.torus_speed *= speed_up_factor;
		}
		SHOW_NEXT_TORUS_PERIOD_MIN -= speed_up_delta;
		SHOW_NEXT_TORUS_PERIOD_MIN = std::max(0.5f, SHOW_NEXT_TORUS_PERIOD_MIN);
		SHOW_NEXT_TORUS_PERIOD_MAX -= speed_up_delta;
		SHOW_NEXT_TORUS_PERIOD_MAX = std::max(1.0f, SHOW_NEXT_TORUS_PERIOD_MAX);

		jump_speed *= speed_up_factor;
	}
}
import sys
sys.path.insert(0, '../python_lib')
import multiprocessing as mp
import vortek


def render(task_queue, status_queue):

    status_queue.put('Stopped')

    functions = {'set_brick_size_power_of_two':               vortek.set_brick_size_power_of_two,
                 'set_minimum_sub_brick_size':                vortek.set_minimum_sub_brick_size,
                 'set_field_from_bifrost_file':               vortek.set_field_from_bifrost_file,
                 'refresh_visibility':                        vortek.refresh_visibility,
                 'refresh_frame':                             vortek.refresh_frame,
                 'enable_autorefresh':                        vortek.enable_autorefresh,
                 'disable_autorefresh':                       vortek.disable_autorefresh,
                 'set_transfer_function_lower_limit':         vortek.set_transfer_function_lower_limit,
                 'set_transfer_function_upper_limit':         vortek.set_transfer_function_upper_limit,
                 'update_transfer_function_lower_node_value': vortek.update_transfer_function_lower_node_value,
                 'update_transfer_function_upper_node_value': vortek.update_transfer_function_upper_node_value,
                 'update_transfer_function_node_value':       vortek.update_transfer_function_node_value,
                 'remove_transfer_function_node':             vortek.remove_transfer_function_node,
                 'use_logarithmic_transfer_function_compone': vortek.use_logarithmic_transfer_function_component,
                 'set_custom_transfer_function_component':    vortek.set_custom_transfer_function_component,
                 'reset_transfer_function_component':         vortek.reset_transfer_function_component,
                 'set_camera_field_of_view':                  vortek.set_camera_field_of_view,
                 'set_clip_plane_distances':                  vortek.set_clip_plane_distances,
                 'use_perspective_camera_projection':         vortek.use_perspective_camera_projection,
                 'use_orthographic_camera_projection':        vortek.use_orthographic_camera_projection,
                 'set_lower_visibility_threshold':            vortek.set_lower_visibility_threshold,
                 'set_upper_visibility_threshold':            vortek.set_upper_visibility_threshold,
                 'set_field_boundary_indicator_creation':     vortek.set_field_boundary_indicator_creation,
                 'set_brick_boundary_indicator_creation':     vortek.set_brick_boundary_indicator_creation,
                 'set_sub_brick_boundary_indicator_creation': vortek.set_sub_brick_boundary_indicator_creation,
                 'bring_window_to_front':                     vortek.bring_window_to_front}

    while task_queue.get() != 'Terminate':

        status = status_queue.get()

        vortek.initialize()

        running = True

        while running:

            while not task_queue.empty():

                task = task_queue.get()

                if task == 'Stop':
                    running = False
                    break
                else:
                    functions[task[0]](*task[1])

            if running:
                running = vortek.step()

        vortek.cleanup()

        status_queue.put('Stopped')

    status = status_queue.get()


class RenderingContext:

    def __init__(self):
        self.task_queue = mp.Queue()
        self.status_queue = mp.Queue()
        self.rendering_process = mp.Process(target=render, args=(self.task_queue, self.status_queue))
        self.rendering_process.start()

    def is_rendering(self):
        return self.status_queue.empty()

    def start_renderer(self):
        if not self.is_rendering():
            self.task_queue.put('Start')

    def stop_renderer(self):
        if self.is_rendering():
            self.task_queue.put('Stop')

    def cleanup(self):
        self.stop_renderer()
        self.task_queue.put('Terminate')
        self.rendering_process.join()

    def set_brick_size_power_of_two(self, brick_size_exponent):
        assert self.is_rendering()
        self.task_queue.put(('set_brick_size_power_of_two', (brick_size_exponent,)))

    def set_minimum_sub_brick_size(self, min_sub_brick_size):
        assert self.is_rendering()
        self.task_queue.put(('set_minimum_sub_brick_size', (min_sub_brick_size,)))

    def set_field_from_bifrost_file(self, field_name, file_base_name):
        assert self.is_rendering()
        self.set_lower_visibility_threshold(5e-4)
        self.task_queue.put(('set_field_from_bifrost_file', (field_name, file_base_name)))

    def refresh_visibility(self):
        assert self.is_rendering()
        self.task_queue.put(('refresh_visibility', ()))

    def refresh_frame(self):
        assert self.is_rendering()
        self.task_queue.put(('refresh_frame', ()))

    def enable_autorefresh(self):
        assert self.is_rendering()
        self.task_queue.put(('enable_autorefresh', ()))

    def disable_autorefresh(self):
        assert self.is_rendering()
        self.task_queue.put(('disable_autorefresh', ()))

    def set_transfer_function_lower_limit(self, lower_limit):
        assert self.is_rendering()
        self.task_queue.put(('set_transfer_function_lower_limit', (lower_limit,)))

    def set_transfer_function_upper_limit(self, upper_limit):
        assert self.is_rendering()
        self.task_queue.put(('set_transfer_function_upper_limit', (upper_limit,)))

    def update_transfer_function_lower_node_value(self, component, value):
        assert self.is_rendering()
        self.task_queue.put(('update_transfer_function_lower_node_value', (component, value)))

    def update_transfer_function_upper_node_value(self, component, value):
        assert self.is_rendering()
        self.task_queue.put(('update_transfer_function_upper_node_value', (component, value)))

    def update_transfer_function_node_value(self, component, node, value):
        assert self.is_rendering()
        self.task_queue.put(('update_transfer_function_node_value', (component, node, value)))

    def remove_transfer_function_node(self, component, node):
        assert self.is_rendering()
        self.task_queue.put(('remove_transfer_function_node', (component, node)))

    def use_logarithmic_transfer_function_component(self, component):
        assert self.is_rendering()
        self.task_queue.put(('use_logarithmic_transfer_function_component', (component,)))

    def set_custom_transfer_function_component(self, component, values):
        assert self.is_rendering()
        self.task_queue.put(('set_custom_transfer_function_component', (component, values.astype('float32'))))

    def reset_transfer_function_component(self, component):
        assert self.is_rendering()
        self.task_queue.put(('reset_transfer_function_component', (component,)))

    def set_camera_field_of_view(self, field_of_view):
        assert self.is_rendering()
        self.task_queue.put(('set_camera_field_of_view', (field_of_view,)))

    def set_clip_plane_distances(self, near_plane_distance, far_plane_distance):
        assert self.is_rendering()
        self.task_queue.put(('set_clip_plane_distances', (near_plane_distance, far_plane_distance)))

    def use_perspective_camera_projection(self):
        assert self.is_rendering()
        self.task_queue.put(('use_perspective_camera_projection', ()))

    def use_orthographic_camera_projection(self):
        assert self.is_rendering()
        self.task_queue.put(('use_orthographic_camera_projection', ()))

    def set_lower_visibility_threshold(self, threshold):
        assert self.is_rendering()
        self.task_queue.put(('set_lower_visibility_threshold', (threshold,)))

    def set_upper_visibility_threshold(self, threshold):
        assert self.is_rendering()
        self.task_queue.put(('set_upper_visibility_threshold', (threshold,)))

    def set_field_boundary_indicator_creation(self, state):
        assert self.is_rendering()
        self.task_queue.put(('set_field_boundary_indicator_creation', (1 if state else 0,)))

    def set_brick_boundary_indicator_creation(self, state):
        assert self.is_rendering()
        self.task_queue.put(('set_brick_boundary_indicator_creation', (1 if state else 0,)))

    def set_sub_brick_boundary_indicator_creation(self, state):
        assert self.is_rendering()
        self.task_queue.put(('set_sub_brick_boundary_indicator_creation', (1 if state else 0,)))

    def bring_window_to_front(self):
        assert self.is_rendering()
        self.task_queue.put(('bring_window_to_front', ()))

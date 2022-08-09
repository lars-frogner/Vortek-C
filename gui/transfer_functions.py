# -*- coding: utf-8 -*-
import numpy as np
import rendering
import matplotlib.pyplot as plt
import matplotlib.lines as mpl_lines
import matplotlib.patches as mpl_patches
import bisect


def HSVs_to_RGBs(hues, saturations, values, reds, greens, blues):

    def helper(n, out):
        k = (n + hues/60) % 6
        out[:] = values*(1 - saturations*np.maximum(0, np.minimum(1, np.minimum(k, 4 - k))))

    helper(5, reds)
    helper(3, greens)
    helper(1, blues)

def HSV_to_RGB(hue, saturation, value):

    def helper(n):
        k = (n + hue/60) % 6
        return value*(1 - saturation*max(0, min(1, min(k, 4 - k))))

    return helper(5), helper(3), helper(1)


class TransferFunctionNode(mpl_patches.Ellipse):

    lock = None  # Only one can be moved at a time

    def __init__(self, *args,
                 x_limits=(0, 1), y_limits=(0, 1),
                 min_node_separation=0,
                 motion_end_callback=lambda node: None,
                 **kwargs):

        super().__init__(*args, **kwargs)

        self.x_limits = x_limits
        self.y_limits = y_limits
        self.min_node_separation = min_node_separation
        self.motion_end_callback = motion_end_callback

        self.lower_limits = [x_limits[0], y_limits[0]]
        self.upper_limits = [x_limits[1], y_limits[1]]

        self.lower_line = None
        self.upper_line = None

        self.is_edge_node = True

        self.click_center_offset = None
        self.background_region = None

        self.display_to_data = None

        self.bin_idx = None

    def set_lower_line(self, lower_line):
        assert isinstance(lower_line, mpl_lines.Line2D)
        self.lower_line = lower_line
        self.is_edge_node = self.lower_line is None or self.upper_line is None

    def set_upper_line(self, upper_line):
        assert isinstance(upper_line, mpl_lines.Line2D)
        self.upper_line = upper_line
        self.is_edge_node = self.lower_line is None or self.upper_line is None

    def set_bin_idx(self, idx):
        self.bin_idx = idx

    def __lt__(self, other):
        return self.center[0] < other.center[0]

    def start_motion(self, click_position):

        self.display_to_data = self.axes.transData.inverted()

        # Update limits for horizontal node movement
        self.lower_limits[0] = self.x_limits[0] if self.lower_line is None else self.lower_line.get_xdata()[0] + self.min_node_separation
        self.upper_limits[0] = self.x_limits[1] if self.upper_line is None else self.upper_line.get_xdata()[1] - self.min_node_separation

        # Take ownership of the lock
        TransferFunctionNode.lock = self

        # Store the offset between the center and the click position
        self.click_center_offset = (self.center[0] - click_position[0],
                                    self.center[1] - click_position[1])

        # Draw everything but the node and connected lines and store the pixel buffer

        self.set_animated(True)
        if self.lower_line is not None:
            self.lower_line.set_animated(True)
        if self.upper_line is not None:
            self.upper_line.set_animated(True)

        self.figure.canvas.draw()

        self.background_region = self.figure.canvas.copy_from_bbox(self.figure.bbox)

        # Redraw just the node and connected lines
        if self.lower_line is not None:
            self.axes.draw_artist(self.lower_line)
        if self.upper_line is not None:
            self.axes.draw_artist(self.upper_line)
        self.axes.draw_artist(self)

        # Blit just the redrawn area
        self.figure.canvas.blit(self.figure.bbox)

    def move_to(self, position):

        # Restore the background region
        self.figure.canvas.restore_region(self.background_region)

        # Update node and line positions and redraw

        if self.lower_line is not None:
            xdata, ydata = self.lower_line.get_data()
            xdata[1] = position[0]
            ydata[1] = position[1]
            self.lower_line.set_data(xdata, ydata)
            self.axes.draw_artist(self.lower_line)

        if self.upper_line is not None:
            xdata, ydata = self.upper_line.get_data()
            xdata[0] = position[0]
            ydata[0] = position[1]
            self.upper_line.set_data(xdata, ydata)
            self.axes.draw_artist(self.upper_line)

        self.center = position
        self.axes.draw_artist(self)

        # Blit just the redrawn area
        self.figure.canvas.blit(self.figure.bbox)

    def end_motion(self):

        self.motion_end_callback(self)

        self.click_center_offset = None

        TransferFunctionNode.lock = None

        self.set_animated(False)
        if self.lower_line is not None:
            self.lower_line.set_animated(False)
        if self.upper_line is not None:
            self.upper_line.set_animated(False)

        self.background_region = None

        # Redraw the full figure
        self.figure.canvas.draw()

    def on_motion(self, mouse_event):

        if TransferFunctionNode.lock is not self:
            return

        click_position = self.display_to_data.transform((mouse_event.x, mouse_event.y))

        new_center = (click_position[0] + self.click_center_offset[0] if not self.is_edge_node else self.center[0],
                      click_position[1] + self.click_center_offset[1])

        # Prevent the node from moving past the edges or adjacent nodes
        limited_new_center = (max(self.lower_limits[0], min(new_center[0], self.upper_limits[0])),
                              max(self.lower_limits[1], min(new_center[1], self.upper_limits[1])))

        self.move_to(limited_new_center)

    def on_release(self, mouse_event):

        if TransferFunctionNode.lock is not self:
            return

        self.end_motion()

    def connect_events(self):
        self.cid_release = self.figure.canvas.mpl_connect('button_release_event', self.on_release)
        self.cid_motion = self.figure.canvas.mpl_connect('motion_notify_event', self.on_motion)

    def disconnect_events(self):
        self.figure.canvas.mpl_disconnect(self.cid_release)
        self.figure.canvas.mpl_disconnect(self.cid_motion)


class TransferFunctionComponent:

    def __init__(self, axes,
                 x_limits=(0, 1), y_limits=(0, 1),
                 aspect_ratio=0.5,
                 start_bin_idx=1, end_bin_idx=254,
                 node_size=5e-2,
                 edge_node_scale=1.5,
                 node_facecolor='k',
                 node_edgecolor='0.8',
                 node_edgewidth=0.5,
                 line_width=1,
                 line_color='k',
                 node_update_callback=lambda node, action, old_bin_idx: None):

        self.axes = axes

        self.x_limits = x_limits
        self.y_limits = y_limits

        self.start_bin_idx = start_bin_idx
        self.end_bin_idx = end_bin_idx

        self.edge_node_scale = edge_node_scale
        self.node_facecolor = node_facecolor
        self.node_edgecolor = node_edgecolor
        self.node_edgewidth = node_edgewidth
        self.line_width = line_width
        self.line_color = line_color

        self.node_update_callback = node_update_callback

        self.is_piecewise = False

        self.x_range = self.x_limits[1] - self.x_limits[0]
        self.y_range = self.y_limits[1] - self.y_limits[0]

        self.axes.set_xlim(*self.x_limits)
        self.axes.set_ylim(*self.y_limits)
        self.axes.set_aspect(aspect_ratio*self.x_range/self.y_range)
        self.axes.set_picker(True)
        self.axes.grid(which='major', zorder=0)
        self.axes.set_zorder(1)

        self.node_height = node_size*self.y_range
        self.node_width = self.node_height*self.axes.get_aspect()

        self.n_bins = 1 + self.end_bin_idx - self.start_bin_idx
        self.bin_width = self.x_range/self.n_bins
        self.min_node_separation = self.bin_width + 1e-6

        n_pixels_x = self.n_bins
        n_pixels_y = int(round(self.n_bins*aspect_ratio))

        self.background_data = np.zeros((n_pixels_y, n_pixels_x, 4))

        self.pixel_x_coordinates = np.linspace(self.x_limits[0], self.x_limits[1], n_pixels_x)
        self.pixel_y_coordinates = np.linspace(self.y_limits[0], self.y_limits[1], n_pixels_y)

        self.background_image = self.axes.imshow(self.background_data,
                                               origin='lower',
                                               extent=(*self.x_limits, *self.y_limits),
                                               aspect=self.axes.get_aspect(),
                                               interpolation='bilinear',
                                               zorder=2)

        self.nodes = []

        self.connect_events()

    def get_bin_idx(self, position):
         return int(self.start_bin_idx + position[0]*(self.end_bin_idx - self.start_bin_idx) + 0.5)

    def get_bin_center(self, idx):
        return self.x_limits[0] + (idx - self.start_bin_idx)*self.bin_width

    def initialize_as_piecewise(self, start_value, end_value):

        assert start_value >= self.y_limits[0]
        assert end_value <= self.y_limits[1]

        self.remove_nodes()

        self.is_piecewise = True

        self.set_lower_value(start_value)
        self.set_upper_value(end_value)

        start_node = TransferFunctionNode((self.x_limits[0], start_value),
                                          x_limits=self.x_limits, y_limits=self.y_limits,
                                          min_node_separation=self.min_node_separation,
                                          motion_end_callback=self.node_placement_action,
                                          width=self.edge_node_scale*self.node_width, height=self.edge_node_scale*self.node_height,
                                          facecolor=self.node_facecolor,
                                          edgecolor=self.node_edgecolor,
                                          linewidth=self.node_edgewidth,
                                          zorder=6)

        end_node   = TransferFunctionNode((self.x_limits[1], end_value),
                                          x_limits=self.x_limits, y_limits=self.y_limits,
                                          min_node_separation=self.min_node_separation,
                                          motion_end_callback=self.node_placement_action,
                                          width=self.edge_node_scale*self.node_width, height=self.edge_node_scale*self.node_height,
                                          facecolor=self.node_facecolor,
                                          edgecolor=self.node_edgecolor,
                                          linewidth=self.node_edgewidth,
                                          zorder=6)

        self.axes.add_artist(start_node)
        start_node.connect_events()

        self.axes.add_artist(end_node)
        end_node.connect_events()

        line = mpl_lines.Line2D([self.x_limits[0], self.x_limits[1]],
                                [start_value, end_value],
                                color=self.line_color,
                                linewidth=self.line_width,
                                zorder=4)
        self.axes.add_artist(line)

        start_node.set_upper_line(line)
        end_node.set_lower_line(line)

        start_node.set_bin_idx(self.start_bin_idx)
        end_node.set_bin_idx(self.end_bin_idx)

        self.nodes = [start_node, end_node]

    def initialize_as_logarithmic(self):

        assert self.y_limits[0] == 0.0
        assert self.y_limits[1] == 1.0

        self.remove_nodes()

        self.is_piecewise = False

        self.set_lower_value(self.y_limits[0])
        self.set_upper_value(self.y_limits[1])

        start_node = TransferFunctionNode((self.x_limits[0], 0),
                                          width=0, height=0,
                                          linewidth=0)

        end_node   = TransferFunctionNode((self.x_limits[1], 1),
                                          width=0, height=0,
                                          linewidth=0)

        self.axes.add_artist(start_node)
        self.axes.add_artist(end_node)

        line = mpl_lines.Line2D(self.pixel_x_coordinates,
                                self.evaluate(),
                                color=self.line_color,
                                linewidth=self.line_width,
                                zorder=4)
        self.axes.add_artist(line)

        start_node.set_upper_line(line)
        end_node.set_lower_line(line)

        self.nodes = [start_node, end_node]

    def add_node(self, position):

        assert self.nodes is not None
        assert position[0] > self.x_limits[0] and position[0] < self.x_limits[1]
        assert position[1] >= self.y_limits[0] and position[1] <= self.y_limits[1]

        node = TransferFunctionNode((position[0], position[1]),
                                    x_limits=self.x_limits, y_limits=self.y_limits,
                                    min_node_separation=self.min_node_separation,
                                    motion_end_callback=self.node_placement_action,
                                    width=self.node_width, height=self.node_height,
                                    facecolor=self.node_facecolor,
                                    edgecolor=self.node_edgecolor,
                                    linewidth=self.node_edgewidth,
                                    zorder=6)

        # Find the correct place in the sorted node list to insert the node
        insertion_idx = bisect.bisect(self.nodes, node)

        self.nodes.insert(insertion_idx, node)

        lower_node = self.nodes[insertion_idx - 1]
        upper_node = self.nodes[insertion_idx + 1]

        new_lower_line = lower_node.upper_line
        new_lower_line.set_data([lower_node.center[0], position[0]],
                                [lower_node.center[1], position[1]])

        new_upper_line = mpl_lines.Line2D([position[0], upper_node.center[0]],
                                          [position[1], upper_node.center[1]],
                                          color=self.line_color,
                                          linewidth=self.line_width,
                                          zorder=4)

        upper_node.set_lower_line(new_upper_line)
        node.set_lower_line(new_lower_line)
        node.set_upper_line(new_upper_line)

        self.axes.add_artist(new_upper_line)
        self.axes.add_artist(node)

        node.connect_events()

        node.start_motion(position)

    def remove_node(self, idx):

        if idx == 0 or idx == len(self.nodes) - 1:
            return

        self.nodes[idx].end_motion()
        self.nodes[idx].remove()
        self.nodes[idx].upper_line.remove()

        self.nodes[idx].lower_line.set_data([self.nodes[idx-1].center[0], self.nodes[idx+1].center[0]],
                                            [self.nodes[idx-1].center[1], self.nodes[idx+1].center[1]])

        self.nodes[idx+1].set_lower_line(self.nodes[idx].lower_line)

        node = self.nodes.pop(idx)

        self.node_update_callback(node, 'remove', None)

        self.draw()

    def remove_nodes(self):

        if len(self.nodes) == 0:
            return

        for node in self.nodes[:-1]:
            node.remove()
            node.upper_line.remove()

        self.nodes[-1].remove()

        self.nodes = []

    def set_lower_value(self, lower_value):
        assert lower_value >= self.y_limits[0]
        self.lower_value = lower_value

    def set_upper_value(self, upper_value):
        assert upper_value <= self.y_limits[1]
        self.upper_value = upper_value

    def evaluate(self, out=None):

        if out is None:
            result = np.zeros(self.pixel_x_coordinates.size)
        else:
            assert out.shape == self.pixel_x_coordinates.shape
            result = out

        if self.is_piecewise:

            for idx in range(1, len(self.nodes)):
                start_bin_idx = self.nodes[idx-1].bin_idx - self.start_bin_idx
                end_bin_idx = self.nodes[idx].bin_idx - self.start_bin_idx
                result[start_bin_idx:(end_bin_idx + 1)] = self.nodes[idx-1].center[1] + \
                                                          ( (self.nodes[idx].center[1] - self.nodes[idx-1].center[1]) \
                                                           /(self.nodes[idx].center[0] - self.nodes[idx-1].center[0])) \
                                                          *(self.pixel_x_coordinates[start_bin_idx:(end_bin_idx + 1)] - self.nodes[idx-1].center[0])

        else:

            result[:] = np.log10(10**0 + (10**1 - 10**0)*(self.pixel_x_coordinates - self.x_limits[0])/self.x_range)

        return result

    def update_background_image(self):
        self.background_image.set_data(self.background_data)

    def draw(self):
        self.axes.figure.canvas.draw()

    def on_pick(self, pick_event):

        if not self.is_piecewise:
            return

        if pick_event.mouseevent.inaxes != self.axes:
            return

        mouse_event = pick_event.mouseevent
        click_position = (mouse_event.xdata, mouse_event.ydata)
        click_bin_idx = self.get_bin_idx(click_position)

        for idx, node in enumerate(self.nodes):
            node_hit, _ = node.contains(mouse_event)
            node_selected = node_hit or node.bin_idx == click_bin_idx
            if node_hit:
                if mouse_event.button != 1 or mouse_event.dblclick:
                    # If a node was clicked with right- or double-click,
                    # remove it
                    self.remove_node(idx)
                else:
                    # If a node was left-clicked, attach it to the mouse pointer
                    node.start_motion(click_position)
                break
            elif node_selected:
                # If the click was inside the bin of an existing node,
                # move the existing node to the click position
                node.start_motion(node.center)
                node.move_to(click_position)
                break

        if not node_selected and mouse_event.button == 1:
            # If the click was within an unused bin,
            # create a new node at the click position
            self.add_node(click_position)

    def node_placement_action(self, node):
        old_bin_idx = node.bin_idx
        node.set_bin_idx(self.get_bin_idx(node.center))
        self.node_update_callback(node, 'place', old_bin_idx)

    def connect_events(self):
        self.cid_pick = self.axes.figure.canvas.mpl_connect('pick_event', self.on_pick)

    def disconnect_events(self):
        self.axes.figure.canvas.mpl_disconnect(self.cid_pick)
        for node in self.nodes:
            node.disconnect_events()


class RGBATransferFunction:

    def __init__(self,
                 red_axes, green_axes, blue_axes, alpha_axes,
                 aspect_ratio=0.5,
                 node_size=5e-2,
                 edge_node_scale=1.5,
                 node_facecolor='k',
                 node_edgecolor='0.8',
                 node_edgewidth=0.5,
                 line_width=1,
                 line_color='k'):

        kwargs = {'y_limits':        (0, 1),
                  'aspect_ratio':    aspect_ratio,
                  'node_size':       node_size,
                  'edge_node_scale': edge_node_scale,
                  'node_facecolor':  node_facecolor,
                  'node_edgecolor':  node_edgecolor,
                  'node_edgewidth':  node_edgewidth,
                  'line_width':      line_width,
                  'line_color':      line_color}

        self.red_component   = TransferFunctionComponent(red_axes,
                                                         node_update_callback=self.red_node_update_action,
                                                         **kwargs)

        self.green_component = TransferFunctionComponent(green_axes,
                                                         node_update_callback=self.green_node_update_action,
                                                         **kwargs)

        self.blue_component  = TransferFunctionComponent(blue_axes,
                                                         node_update_callback=self.blue_node_update_action,
                                                         **kwargs)

        self.alpha_component = TransferFunctionComponent(alpha_axes,
                                                         node_update_callback=self.alpha_node_update_action,
                                                         **kwargs)

        self.red_component  .initialize_as_piecewise(0, 1)
        self.green_component.initialize_as_piecewise(0, 1)
        self.blue_component .initialize_as_piecewise(0, 1)
        self.alpha_component.initialize_as_piecewise(0, 1)

        self.components = [self.red_component, self.green_component, self.blue_component, self.alpha_component]

        self.red_data   = self.red_component.evaluate()
        self.green_data = self.green_component.evaluate()
        self.blue_data  = self.blue_component.evaluate()
        self.alpha_data = self.alpha_component.evaluate()

        self.red_component  .background_data[:, :, 0] = self.red_component.pixel_y_coordinates[:, np.newaxis]
        self.green_component.background_data[:, :, 1] = self.green_component.pixel_y_coordinates[:, np.newaxis]
        self.blue_component .background_data[:, :, 2] = self.blue_component.pixel_y_coordinates[:, np.newaxis]
        self.alpha_component.background_data[:, :, 3] = self.alpha_component.pixel_y_coordinates[:, np.newaxis]

        self.update_other_background_components_for_red()
        self.update_other_background_components_for_green()
        self.update_other_background_components_for_blue()
        self.update_other_background_components_for_alpha()

    @staticmethod
    def get_component_names():
        return ['Red', 'Green', 'Blue', 'Alpha']

    def reset(self):
        rendering.session.disable_autorefresh()
        self.make_red_component_piecewise()
        self.make_green_component_piecewise()
        self.make_blue_component_piecewise()
        rendering.session.enable_autorefresh()
        self.make_alpha_component_piecewise()

    def make_red_component_piecewise(self):
        self.red_component.initialize_as_piecewise(0, 1)
        self.update_red_component_background()
        rendering.session.reset_transfer_function_component(0)

    def make_green_component_piecewise(self):
        self.green_component.initialize_as_piecewise(0, 1)
        self.update_green_component_background()
        rendering.session.reset_transfer_function_component(1)

    def make_blue_component_piecewise(self):
        self.blue_component.initialize_as_piecewise(0, 1)
        self.update_blue_component_background()
        rendering.session.reset_transfer_function_component(2)

    def make_alpha_component_piecewise(self):
        self.alpha_component.initialize_as_piecewise(0, 1)
        self.update_alpha_component_background()
        rendering.session.reset_transfer_function_component(3)

    def make_red_component_logarithmic(self):
        self.red_component.initialize_as_logarithmic()
        self.update_red_component_background()
        rendering.session.use_logarithmic_transfer_function_component(0)

    def make_green_component_logarithmic(self):
        self.green_component.initialize_as_logarithmic()
        self.update_green_component_background()
        rendering.session.use_logarithmic_transfer_function_component(1)

    def make_blue_component_logarithmic(self):
        self.blue_component.initialize_as_logarithmic()
        self.update_blue_component_background()
        rendering.session.use_logarithmic_transfer_function_component(2)

    def make_alpha_component_logarithmic(self):
        self.alpha_component.initialize_as_logarithmic()
        self.update_alpha_component_background()
        rendering.session.use_logarithmic_transfer_function_component(3)

    def set_lower_value(self, idx, lower_value):
        self.components[idx].set_lower_value(lower_value)
        rendering.session.update_transfer_function_lower_node_value(idx, lower_value)

    def set_upper_value(self, idx, upper_value):
        self.components[idx].set_upper_value(upper_value)
        rendering.session.update_transfer_function_upper_node_value(idx, upper_value)

    def get_value_lower_limit(self, idx):
        return self.components[idx].y_limits[0]

    def get_value_upper_limit(self, idx):
        return self.components[idx].y_limits[1]

    def get_lower_value(self, idx):
        return self.components[idx].lower_value

    def get_upper_value(self, idx):
        return self.components[idx].upper_value

    def update_other_background_components_for_red(self):
        self.red_component.background_data[:, :, 1] = self.green_data[np.newaxis, :]
        self.red_component.background_data[:, :, 2] = self.blue_data[np.newaxis, :]
        self.red_component.background_data[:, :, 3] = self.alpha_data[np.newaxis, :]
        self.red_component.update_background_image()
        self.red_component.draw()

    def update_other_background_components_for_green(self):
        self.green_component.background_data[:, :, 0] = self.red_data[np.newaxis, :]
        self.green_component.background_data[:, :, 2] = self.blue_data[np.newaxis, :]
        self.green_component.background_data[:, :, 3] = self.alpha_data[np.newaxis, :]
        self.green_component.update_background_image()
        self.green_component.draw()

    def update_other_background_components_for_blue(self):
        self.blue_component.background_data[:, :, 0] = self.red_data[np.newaxis, :]
        self.blue_component.background_data[:, :, 1] = self.green_data[np.newaxis, :]
        self.blue_component.background_data[:, :, 3] = self.alpha_data[np.newaxis, :]
        self.blue_component.update_background_image()
        self.blue_component.draw()

    def update_other_background_components_for_alpha(self):
        self.alpha_component.background_data[:, :, 0] = self.red_data[np.newaxis, :]
        self.alpha_component.background_data[:, :, 1] = self.green_data[np.newaxis, :]
        self.alpha_component.background_data[:, :, 2] = self.blue_data[np.newaxis, :]
        self.alpha_component.update_background_image()
        self.alpha_component.draw()

    def update_red_component_background(self):
        self.red_component.evaluate(out=self.red_data)
        self.update_other_background_components_for_green()
        self.update_other_background_components_for_blue()
        self.update_other_background_components_for_alpha()
        self.red_component.update_background_image()

    def update_green_component_background(self):
        self.green_component.evaluate(out=self.green_data)
        self.update_other_background_components_for_red()
        self.update_other_background_components_for_blue()
        self.update_other_background_components_for_alpha()
        self.green_component.update_background_image()

    def update_blue_component_background(self):
        self.blue_component.evaluate(out=self.blue_data)
        self.update_other_background_components_for_red()
        self.update_other_background_components_for_green()
        self.update_other_background_components_for_alpha()
        self.blue_component.update_background_image()

    def update_alpha_component_background(self):
        self.alpha_component.evaluate(out=self.alpha_data)
        self.update_other_background_components_for_red()
        self.update_other_background_components_for_green()
        self.update_other_background_components_for_blue()
        self.alpha_component.update_background_image()

    def update_node_for_renderer(self, component_idx, node, action, old_bin_idx):

        if action == 'remove':
            rendering.session.remove_transfer_function_node(component_idx, node.bin_idx)
        else:
            if old_bin_idx is None:
                rendering.session.update_transfer_function_node_value(component_idx, node.bin_idx, node.center[1])
            else:
                if node.bin_idx != old_bin_idx:
                    rendering.session.disable_autorefresh()
                    rendering.session.remove_transfer_function_node(component_idx, old_bin_idx)
                    rendering.session.enable_autorefresh()
                rendering.session.update_transfer_function_node_value(component_idx, node.bin_idx, node.center[1])

    def red_node_update_action(self, node, action, old_bin_idx):
        self.update_red_component_background()
        self.update_node_for_renderer(0, node, action, old_bin_idx)

    def green_node_update_action(self, node, action, old_bin_idx):
        self.update_green_component_background()
        self.update_node_for_renderer(1, node, action, old_bin_idx)

    def blue_node_update_action(self, node, action, old_bin_idx):
        self.update_blue_component_background()
        self.update_node_for_renderer(2, node, action, old_bin_idx)

    def alpha_node_update_action(self, node, action, old_bin_idx):
        self.update_alpha_component_background()
        self.update_node_for_renderer(3, node, action, old_bin_idx)


class HSVATransferFunction:

    def __init__(self,
                 hue_axes, saturation_axes, value_axes, alpha_axes,
                 aspect_ratio=0.5,
                 node_size=5e-2,
                 edge_node_scale=1.5,
                 node_facecolor='k',
                 node_edgecolor='0.8',
                 node_edgewidth=0.5,
                 line_width=1,
                 line_color='k'):

        kwargs = {'aspect_ratio':    aspect_ratio,
                  'node_size':       node_size,
                  'edge_node_scale': edge_node_scale,
                  'node_facecolor':  node_facecolor,
                  'node_edgecolor':  node_edgecolor,
                  'node_edgewidth':  node_edgewidth,
                  'line_width':      line_width,
                  'line_color':      line_color}

        self.hue_component        = TransferFunctionComponent(hue_axes,
                                                              y_limits=(0, 360),
                                                              node_update_callback=self.hue_node_update_action,
                                                              **kwargs)

        self.saturation_component = TransferFunctionComponent(saturation_axes,
                                                              y_limits=(0, 1),
                                                              node_update_callback=self.saturation_node_update_action,
                                                              **kwargs)

        self.value_component      = TransferFunctionComponent(value_axes,
                                                              y_limits=(0, 1),
                                                              node_update_callback=self.value_node_update_action,
                                                              **kwargs)

        self.alpha_component      = TransferFunctionComponent(alpha_axes,
                                                              y_limits=(0, 1),
                                                              node_update_callback=self.alpha_node_update_action,
                                                              **kwargs)

        self.hue_component       .initialize_as_piecewise(0, 0)
        self.saturation_component.initialize_as_piecewise(0, 0)
        self.value_component     .initialize_as_piecewise(0, 1)
        self.alpha_component     .initialize_as_piecewise(0, 1)

        self.components = [self.hue_component, self.saturation_component, self.value_component, self.alpha_component]

        self.hue_data        = self.hue_component.evaluate()
        self.saturation_data = self.saturation_component.evaluate()
        self.value_data      = self.value_component.evaluate()
        self.alpha_data      = self.alpha_component.evaluate()

        self.red_data   = np.zeros(self.hue_data.shape)
        self.green_data = np.zeros(self.hue_data.shape)
        self.blue_data  = np.zeros(self.hue_data.shape)

        self.HSVA_lower_values = np.zeros(4)
        self.HSVA_upper_values = np.zeros(4)
        self.RGBA_lower_values = np.zeros(4)
        self.RGBA_upper_values = np.zeros(4)

        self.alpha_component.background_data[:, :, 3] = self.alpha_component.pixel_y_coordinates[:, np.newaxis]

        self.update_background_components_for_hue()
        self.update_background_components_for_saturation()
        self.update_background_components_for_value()
        self.update_background_components_for_alpha()

    @staticmethod
    def get_component_names():
        return ['Hue', 'Saturation', 'Value', 'Alpha']

    def reset(self):
        self.hue_component       .initialize_as_piecewise(0, 0)
        self.saturation_component.initialize_as_piecewise(0, 0)
        self.value_component     .initialize_as_piecewise(0, 1)
        self.alpha_component     .initialize_as_piecewise(0, 1)

        self.hue_component       .evaluate(out=self.hue_data)
        self.saturation_component.evaluate(out=self.saturation_data)
        self.value_component     .evaluate(out=self.value_data)
        self.alpha_component     .evaluate(out=self.alpha_data)

        self.update_background_components_for_hue()
        self.update_background_components_for_saturation()
        self.update_background_components_for_value()
        self.update_background_components_for_alpha()

        rendering.session.disable_autorefresh()
        rendering.session.reset_transfer_function_component(0)
        rendering.session.reset_transfer_function_component(1)
        rendering.session.reset_transfer_function_component(2)
        rendering.session.enable_autorefresh()
        rendering.session.reset_transfer_function_component(3)

    def make_hue_component_piecewise(self):
        self.hue_component.initialize_as_piecewise(0, 0)
        self.update_hue_component_background()
        self.update_RGB_data_for_renderer()

    def make_saturation_component_piecewise(self):
        self.saturation_component.initialize_as_piecewise(0, 0)
        self.update_saturation_component_background()
        self.update_RGB_data_for_renderer()

    def make_value_component_piecewise(self):
        self.value_component.initialize_as_piecewise(0, 1)
        self.update_value_component_background()
        self.update_RGB_data_for_renderer()

    def make_alpha_component_piecewise(self):
        self.alpha_component.initialize_as_piecewise(0, 1)
        self.update_alpha_component_background()
        rendering.session.reset_transfer_function_component(3)

    def make_saturation_component_logarithmic(self):
        self.saturation_component.initialize_as_logarithmic()
        self.update_saturation_component_background()
        self.update_RGB_data_for_renderer()

    def make_value_component_logarithmic(self):
        self.value_component.initialize_as_logarithmic()
        self.update_value_component_background()
        self.update_RGB_data_for_renderer()

    def make_alpha_component_logarithmic(self):
        self.alpha_component.initialize_as_logarithmic()
        self.update_alpha_component_background()
        rendering.session.use_logarithmic_transfer_function_component(3)

    def set_lower_value(self, idx, lower_value):

        self.components[idx].set_lower_value(lower_value)

        if idx != 3:
            red_lower_value, green_lower_value, blue_lower_value = HSV_to_RGB(self.hue_component.lower_value,
                                                                              self.saturation_component.lower_value,
                                                                              self.value_component.lower_value)
            rendering.session.disable_autorefresh()
            rendering.session.update_transfer_function_lower_node_value(0, red_lower_value)
            rendering.session.update_transfer_function_lower_node_value(1, green_lower_value)
            rendering.session.enable_autorefresh()
            rendering.session.update_transfer_function_lower_node_value(2, blue_lower_value)
        else:
            rendering.session.update_transfer_function_lower_node_value(idx, lower_value)

    def set_upper_value(self, idx, upper_value):

        self.components[idx].set_upper_value(upper_value)

        if idx != 3:
            red_upper_value, green_upper_value, blue_upper_value = HSV_to_RGB(self.hue_component.upper_value,
                                                                              self.saturation_component.upper_value,
                                                                              self.value_component.upper_value)
            rendering.session.disable_autorefresh()
            rendering.session.update_transfer_function_upper_node_value(0, red_upper_value)
            rendering.session.update_transfer_function_upper_node_value(1, green_upper_value)
            rendering.session.enable_autorefresh()
            rendering.session.update_transfer_function_upper_node_value(2, blue_upper_value)
        else:
            rendering.session.update_transfer_function_upper_node_value(idx, upper_value)

    def get_value_lower_limit(self, idx):
        return self.components[idx].y_limits[0]

    def get_value_upper_limit(self, idx):
        return self.components[idx].y_limits[1]

    def get_lower_value(self, idx):
        return self.components[idx].lower_value

    def get_upper_value(self, idx):
        return self.components[idx].upper_value

    def compute_RGB_data(self):
        HSVs_to_RGBs(self.hue_data, self.saturation_data, self.value_data,
                     self.red_data, self.green_data, self.blue_data)

    def update_RGB_data_for_renderer(self):
        self.compute_RGB_data()
        rendering.session.disable_autorefresh()
        rendering.session.set_custom_transfer_function_component(0, self.red_data)
        rendering.session.set_custom_transfer_function_component(1, self.green_data)
        rendering.session.enable_autorefresh()
        rendering.session.set_custom_transfer_function_component(2, self.blue_data)

    def update_background_components_for_hue(self):

        HSVs_to_RGBs(self.hue_component.pixel_y_coordinates[:, np.newaxis],
                     self.saturation_data[np.newaxis, :],
                     self.value_data[np.newaxis, :],
                     self.hue_component.background_data[:, :, 0],
                     self.hue_component.background_data[:, :, 1],
                     self.hue_component.background_data[:, :, 2])

        self.update_alpha_background_component_for_hue()

    def update_background_components_for_saturation(self):

        HSVs_to_RGBs(self.hue_data[np.newaxis, :],
                     self.saturation_component.pixel_y_coordinates[:, np.newaxis],
                     self.value_data[np.newaxis, :],
                     self.saturation_component.background_data[:, :, 0],
                     self.saturation_component.background_data[:, :, 1],
                     self.saturation_component.background_data[:, :, 2])

        self.update_alpha_background_component_for_saturation()

    def update_background_components_for_value(self):

        HSVs_to_RGBs(self.hue_data[np.newaxis, :],
                     self.saturation_data[np.newaxis, :],
                     self.value_component.pixel_y_coordinates[:, np.newaxis],
                     self.value_component.background_data[:, :, 0],
                     self.value_component.background_data[:, :, 1],
                     self.value_component.background_data[:, :, 2])

        self.update_alpha_background_component_for_value()

    def update_alpha_background_component_for_hue(self):
        self.hue_component.background_data[:, :, 3] = self.alpha_data[np.newaxis, :]
        self.hue_component.update_background_image()
        self.hue_component.draw()

    def update_alpha_background_component_for_saturation(self):
        self.saturation_component.background_data[:, :, 3] = self.alpha_data[np.newaxis, :]
        self.saturation_component.update_background_image()
        self.saturation_component.draw()

    def update_alpha_background_component_for_value(self):
        self.value_component.background_data[:, :, 3] = self.alpha_data[np.newaxis, :]
        self.value_component.update_background_image()
        self.value_component.draw()

    def update_background_components_for_alpha(self):

        HSVs_to_RGBs(self.hue_data[np.newaxis, :],
                     self.saturation_data[np.newaxis, :],
                     self.value_data[np.newaxis, :],
                     self.alpha_component.background_data[:, :, 0],
                     self.alpha_component.background_data[:, :, 1],
                     self.alpha_component.background_data[:, :, 2])

        self.alpha_component.update_background_image()
        self.alpha_component.draw()

    def update_hue_component_background(self):
        self.hue_component.evaluate(out=self.hue_data)
        self.update_background_components_for_saturation()
        self.update_background_components_for_value()
        self.update_background_components_for_alpha()
        self.hue_component.update_background_image()

    def update_saturation_component_background(self):
        self.saturation_component.evaluate(out=self.saturation_data)
        self.update_background_components_for_hue()
        self.update_background_components_for_value()
        self.update_background_components_for_alpha()
        self.saturation_component.update_background_image()

    def update_value_component_background(self):
        self.value_component.evaluate(out=self.value_data)
        self.update_background_components_for_hue()
        self.update_background_components_for_saturation()
        self.update_background_components_for_alpha()
        self.value_component.update_background_image()

    def update_alpha_component_background(self):
        self.alpha_component.evaluate(out=self.alpha_data)
        self.update_alpha_background_component_for_hue()
        self.update_alpha_background_component_for_saturation()
        self.update_alpha_background_component_for_value()
        self.alpha_component.update_background_image()

    def hue_node_update_action(self, node, action, old_bin_idx):
        self.update_hue_component_background()
        self.update_RGB_data_for_renderer()

    def saturation_node_update_action(self, node, action, old_bin_idx):
        self.update_saturation_component_background()
        self.update_RGB_data_for_renderer()

    def value_node_update_action(self, node, action, old_bin_idx):
        self.update_value_component_background()
        self.update_RGB_data_for_renderer()

    def alpha_node_update_action(self, node, action, old_bin_idx):

        self.update_alpha_component_background()

        if action == 'remove':
            rendering.session.remove_transfer_function_node(3, node.bin_idx)
        else:
            if old_bin_idx is None:
                rendering.session.update_transfer_function_node_value(3, node.bin_idx, node.center[1])
            else:
                if node.bin_idx != old_bin_idx:
                    rendering.session.disable_autorefresh()
                    rendering.session.remove_transfer_function_node(3, old_bin_idx)
                    rendering.session.enable_autorefresh()
                rendering.session.update_transfer_function_node_value(3, node.bin_idx, node.center[1])

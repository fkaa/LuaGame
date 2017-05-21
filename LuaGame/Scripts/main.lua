state = "menu"
selected_map = 0
current_map = nil
current_map_name = nil
current_palette = 0

blank = load_sprite("White")
player_sprite = load_sprite("Player")
bullet_sprite = load_sprite("Bullet")

tiles = {
	{ sprite = load_sprite("Placeholder"), physics = false },
	{ sprite = load_sprite("MetalWall"), physics = true },
	{ sprite = load_sprite("Dirt"), physics = false }
}

maps = load_maps()

players = {}
bullets = {}

Player = {}
Player.__index = Player

function Player:new(x, y)
   local player = {}
   setmetatable(player, Player)
   player.x = x
   player.y = y
   player.angle = 0
   player.reload = 0.5
   player.radius = 0.4
   return player
end

function Player:update(dt)
	if self.reload > 0 then
		self.reload = self.reload - dt
	end
	if self.reload < 0 then
		self.reload = 0
	end
end

function Player:shoot()
	if self.reload == 0 then
		-- shoot
		table.insert(bullets, {
			x = self.x + math.sin(self.angle) * 0.5,
			y = self.y + math.cos(self.angle) * 0.5,
			angle = self.angle
		})

		self.reload = 0.5
	end
end

function Player:forward(speed, dt)
	self.x = self.x + math.sin(self.angle) * speed * dt
	self.y = self.y + math.cos(self.angle) * speed * dt
end

function Player:right(speed, dt)
	self.angle = self.angle - speed * dt
	if self.angle < 0 then
		self.angle = self.angle + math.pi * 2
	end
end

function Player:left(speed, dt)
	self.angle = self.angle + speed * dt
	if self.angle > math.pi * 2 then
		self.angle = self.angle - math.pi * 2
	end
end

function init()
end

function update(dt)
	if state == "menu" then
		local maplen = 6

		if WP or UpP then
			selected_map = selected_map - 3
			if selected_map < 0 then
				selected_map = selected_map + maplen
			end
		elseif AP or LeftP then
			selected_map = selected_map - 1
			if selected_map < 0 then
				selected_map = selected_map + maplen
			end
		elseif SP or DownP then
			selected_map = selected_map + 3
			if selected_map >= maplen then
				selected_map = selected_map - maplen
			end
		elseif DP or RightP then
			selected_map = selected_map + 1
			if selected_map >= maplen then
				selected_map = selected_map - maplen
			end
		end

		if EP then
			state = "editor"
		elseif SpaceP then
			state = "game"

			players[1] = Player:new(current_map[1] + 0.5, current_map[2] + 0.5)
			players[2] = Player:new(current_map[3] + 0.5, current_map[4] + 0.5)
		end
	elseif state == "editor" then
		if D0 then current_palette = 0
		elseif D1 then current_palette = 1
		elseif D2 then current_palette = 2
		elseif D3 then current_palette = 3
		elseif D4 then current_palette = 4
		elseif D5 then current_palette = 5
		end

		if current_palette > 2 then
			current_palette = 2
		end

		hover_x = math.floor(nmx)
		hover_y = math.floor(nmy)

		if mbLeft then
			local idx = hover_y * 16 + hover_x

			current_map[5 + idx] = current_palette
		end

		if WP then
			current_map[1] = hover_x
			current_map[2] = hover_y
		end
		if DP then
			current_map[3] = hover_x
			current_map[4] = hover_y
		end

		if SP then
			print("save(" .. current_map_name .. ")")
			save_map(current_map, current_map_name)
		end

		if QP then
			state = "menu"
		end
	elseif state == "game" then
		local vel = 2.1
		if W then
			players[1]:forward(vel, dt)
		elseif S then
			players[1]:forward(-vel, dt)
		end

		local ang_vel = 2.5
		if A then
			players[1]:left(ang_vel, dt)
		elseif D then
			players[1]:right(ang_vel, dt)
		end

		if Space then
			players[1]:shoot()
		end

		players[1]:update(dt)
		
		local bul_speed = 4
		for i,v in ipairs(bullets) do
			v.x = v.x + math.sin(v.angle) * bul_speed * dt
			v.y = v.y + math.cos(v.angle) * bul_speed * dt
		end
	end
end

function render(dt)
	if state == "menu" then
		local padding = 20
		local ipadding = 40

		local iw = (width - padding * 2 - ipadding * 2) / 3
		local ih = (height - padding * 2 - ipadding * 2) / 3

		local x = padding
		local y = padding

		local row = 0

		local i = 0
		for k,v in pairs(maps) do
			local col = 0x797979ff
			if selected_map == i then
				current_map = v
				current_map_name = k
				col = 0x89dd89ff
			end

			local sp1x = v[1]
			local sp1y = v[2]
			local sp2x = v[3]
			local sp2y = v[4]

			draw_sprite(blank, x - 2, y - 2, iw + 4, ih + 4, col, 0)

			local mulw = iw / 16
			local mulh = ih / 9
			
			for Q=0,(16*9-1) do
				local idx = 5 + Q
				local tile = tiles[v[idx] + 1]

				local tx = Q % 16
				local ty = Q // 16

				if tile ~= nil then
					if (tx == sp1x and ty == sp1y) or (tx == sp2x and ty == sp2y) then
						draw_sprite(blank, x + tx * mulw, y + ty * mulh, mulw, mulh, 0x89dd89ff, 0)
					elseif tile.physics then
						draw_sprite(blank, x + tx * mulw, y + ty * mulh, mulw, mulh, 0xdededeff, 0)
					else
						draw_sprite(blank, x + tx * mulw, y + ty * mulh, mulw, mulh, 0x343434ff, 0)
					end
				end
			end
			
			draw_text(k, x + 60, y + ih, col)

			x = x + iw + ipadding
			i = i + 1

			if i % 3 == 0 then
				row = row + 1

				x = padding
				y = y + ih + ipadding
			end

		end

		draw_text("[Space] - Play map", padding + 40, height - padding*2, 0x797979ff)
		draw_text("[E] - Edit map", padding + 40 + 250, height - padding*2, 0x797979ff)
	elseif state == "editor" then
		local sp1x = current_map[1]
		local sp1y = current_map[2]
		local sp2x = current_map[3]
		local sp2y = current_map[4]

		local mulw = width / 16
		local mulh = height / 9

		for i=0,(16*9-1) do
			local idx = 5 + i
			local tile = tiles[current_map[idx] + 1]

			local x = i % 16
			local y = i // 16

			if tile ~= nil then
				draw_sprite(tile.sprite, x*mulw, y*mulh, mulw, mulh, 0xffffffff, 0)
			end
		end

		draw_sprite(blank, sp1x*mulw + 15, sp1y*mulh + 15, mulw - 30, mulh - 30, 0x89dd8934, 0)
		draw_sprite(blank, sp2x*mulw + 15, sp2y*mulh + 15, mulw - 30, mulh - 30, 0x89dd8934, 0)

		hover_x = math.floor(nmx)
		hover_y = math.floor(nmy)

		draw_sprite(tiles[current_palette + 1].sprite, hover_x*mulw, hover_y*mulh, mulw, mulh, 0xccffcc55, 0)
	elseif state == "game" then
	
		local mulw = width / 16
		local mulh = height / 9

		for i=0,(16*9-1) do
			local idx = 5 + i
			local tile = tiles[current_map[idx] + 1]

			local x = i % 16
			local y = i // 16

			if tile ~= nil then
				draw_sprite(tile.sprite, x*mulw, y*mulh, mulw, mulh, 0xffffffff, 0)
			end
		end

		for i,v in ipairs(players) do
			draw_sprite(player_sprite, v.x*mulw, v.y*mulh, mulw, mulh, 0xffffffff, -v.angle)
		end

		for i,v in ipairs(bullets) do
			draw_sprite(bullet_sprite, v.x*mulw, v.y*mulh, mulw, mulh, 0xffffffff, -v.angle)
		end
	end
end

function click(id)
end

function key(id)
end
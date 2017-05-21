state = "menu"
selected_map = 0
current_map = nil
current_map_name = nil
current_palette = 0

blank = load_sprite("White")

tiles = {
	{ sprite = load_sprite("Placeholder"), physics = false },
	{ sprite = load_sprite("MetalWall"), physics = true }
}

entities = {}

maps = load_maps()

function init()
	bs = load_sprite("Body")
	bt = load_sprite("Turret")
end

function update(dt)
	if state == "menu" then
		local maplen = 6

		if WP or UPP then
			selected_map = selected_map - 3
			if selected_map < 0 then
				selected_map = selected_map + maplen
			end
		elseif AP or LEFTP then
			selected_map = selected_map - 1
			if selected_map < 0 then
				selected_map = selected_map + maplen
			end
		elseif SP or DOWNP then
			selected_map = selected_map + 3
			if selected_map >= maplen then
				selected_map = selected_map - maplen
			end
		elseif DP or RIGHTP then
			selected_map = selected_map + 1
			if selected_map >= maplen then
				selected_map = selected_map - maplen
			end
		end

		if EP then
			state = "editor"
		elseif SPACEP then
			state = "game"
		end
	elseif state == "editor" then
		if D0 then current_palette = 0
		elseif D1 then current_palette = 1
		elseif D2 then current_palette = 2
		elseif D3 then current_palette = 3
		elseif D4 then current_palette = 4
		elseif D5 then current_palette = 5
		end

		if current_palette > 1 then
			current_palette = 1
		end

		hover_x = math.floor(nmx)
		hover_y = math.floor(nmy)

		if mbLeft then
			local idx = hover_y * 16 + hover_x

			current_map[5 + idx] = current_palette
		end

		if SP then
			print("save(" .. current_map_name .. ")")
			save_map(current_map, current_map_name)
		end
	elseif state == "game" then
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

		hover_x = math.floor(nmx)
		hover_y = math.floor(nmy)

		draw_sprite(tiles[current_palette + 1].sprite, hover_x*mulw, hover_y*mulh, mulw, mulh, 0xccffcc55, 0)


	elseif state == "game" then
	end
end

function click(id)
end

function key(id)
end
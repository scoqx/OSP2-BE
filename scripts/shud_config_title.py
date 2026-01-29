import os

# Параметры заголовка
name = "<name>"
contact_type = "<contact type>"	# например Discord, Email, etc.
contact_address = "<address>"	# например Scoqx#1234 или email

header = f"""###############################################
##          OSP2-BE SuperHUD config          ##
##           created by: {name}              ##
##       {contact_type}: {contact_address}           ##
###############################################
"""

# Находим путь к папке assets/hud на уровень выше текущей
parent_folder = os.path.abspath(os.path.join(os.getcwd(), ".."))
hud_folder = os.path.join(parent_folder, "assets", "hud")

if not os.path.isdir(hud_folder):
	print(f"Folder not found: {hud_folder}")
else:
	for filename in os.listdir(hud_folder):
		if filename.endswith(".cfg"):
			path = os.path.join(hud_folder, filename)

			with open(path, "r", encoding="utf-8") as f:
				content = f.read()

			# Проверяем, есть ли уже заголовок
			if content.startswith("###############################################"):
				print(f"Header already exists in {filename}, skipping.")
				continue

			# Добавляем заголовок
			with open(path, "w", encoding="utf-8") as f:
				f.write(header + "\n" + content)

			print(f"Header added to {filename}")

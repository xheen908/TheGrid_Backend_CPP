from PIL import Image
import os

files = [
    (r"C:/Users/xheen908/.gemini/antigravity/brain/638c9cbe-f319-4ee6-bee6-f4b7fb13c40b/speed_potion_icon_v1_1770208652130.png", "item_speed_potion.jpg"),
    (r"C:/Users/xheen908/.gemini/antigravity/brain/638c9cbe-f319-4ee6-bee6-f4b7fb13c40b/strength_potion_icon_v1_1770208665477.png", "item_strength_potion.jpg"),
    (r"C:/Users/xheen908/.gemini/antigravity/brain/638c9cbe-f319-4ee6-bee6-f4b7fb13c40b/intellect_potion_icon_v1_1770208682239.png", "item_intellect_potion.jpg"),
    (r"C:/Users/xheen908/.gemini/antigravity/brain/638c9cbe-f319-4ee6-bee6-f4b7fb13c40b/luck_potion_icon_v1_1770208695065.png", "item_luck_potion.jpg")
]

target_dir = r"e:\TheGrid\frontend\Assets\UI"

for src, name in files:
    if os.path.exists(src):
        img = Image.open(src)
        rgb_img = img.convert('RGB')
        target_path = os.path.join(target_dir, name)
        rgb_img.save(target_path, "JPEG", quality=90)
        print(f"Saved {target_path}")
    else:
        print(f"Source not found: {src}")

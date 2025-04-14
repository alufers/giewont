import argparse
import json
import os
import math
from PIL import Image

def split_spritesheet(image_path, json_path, output_dir):
    os.makedirs(output_dir, exist_ok=True)
    with open(json_path, 'r') as f:
        metadata = json.load(f)
    image = Image.open(image_path)

    for name, (x, y, w, h) in metadata.items():
        frame = image.crop((x, y, x + w, y + h))
        frame.save(os.path.join(output_dir, f"{name}.png"))
    print(f"Split {len(metadata)} frames to '{output_dir}'.")

def combine_sprites(output_dir: str, image_path:str, json_path:str, margin: int = 0):
    frames: dict[str, Image.Image] = {}
    for file in sorted(os.listdir(output_dir)):
        if file.endswith('.png'):
            img = Image.open(os.path.join(output_dir, file))
            name = os.path.splitext(file)[0]
            if margin > 0:
                # add margin to the image
                img_with_margin = Image.new('RGBA', (img.width + margin * 2, img.height + margin * 2), (0, 0, 0, 0))
                img_with_margin.paste(img, (margin, margin))
                img = img_with_margin

            frames[name] = img
    widths, heights = [], []
    for img in frames.values():
        widths.append(img.width)
        heights.append(img.height)
    max_width = max(widths)
    max_height = max(heights)

    column_count = math.ceil(math.sqrt(len(frames)))
    row_count = math.ceil(len(frames) / column_count)
    total_width = column_count * max_width
    total_height = row_count * max_height

    spritesheet = Image.new('RGBA', (total_width, total_height))
    metadata = {}
    for index, (name, img) in enumerate(frames.items()):
        x = (index % column_count) * max_width
        y = (index // column_count) * max_height
        spritesheet.paste(img, (x, y))
        metadata[name] = (x, y, img.width, img.height)

    spritesheet.save(image_path)
    with open(json_path, 'w') as f:
        json.dump(metadata, f, indent=2)
    print(f"Combined {len(frames)} frames into '{image_path}' with metadata '{json_path}'.")

def main():
    parser = argparse.ArgumentParser(description="Spritesheet tool (split/combine).")
    subparsers = parser.add_subparsers(dest="command", required=True)

    split_parser = subparsers.add_parser("split", help="Split a spritesheet.")
    split_parser.add_argument("image", help="Path to spritesheet PNG.")
    split_parser.add_argument("json", help="Path to JSON metadata.")
    split_parser.add_argument("output", help="Directory to save frames.")

    combine_parser = subparsers.add_parser("combine", help="Combine frames into a spritesheet.")
    combine_parser.add_argument("input", help="Directory with PNG frames.")
    combine_parser.add_argument("image", help="Output spritesheet PNG path.")
    combine_parser.add_argument("json", help="Output JSON metadata path.")
    combine_parser.add_argument("--margin", type=int, default=0, help="Margin to add around each frame.")

    args = parser.parse_args()
    if args.command == "split":
        split_spritesheet(args.image, args.json, args.output)
    elif args.command == "combine":
        combine_sprites(args.input, args.image, args.json, args.margin)

if __name__ == "__main__":
    main()

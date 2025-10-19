#include "uno.h"
#include "ui/draw/draw.h"

gpu_size calculate_label_size(text_ui_config text_config){
    if (!text_config.text || text_config.font_size==0) return (gpu_size){0,0};
    int num_lines = 1;
    int num_chars = 0;
    int local_num_chars = 0;
    for (const char* p = text_config.text; *p; ++p){
        if (*p == '\n'){
            if (local_num_chars > num_chars)
                num_chars = local_num_chars;
            num_lines++;
            local_num_chars = 0;
        }
        else 
            local_num_chars++;
    }
    if (local_num_chars > num_chars)
        num_chars = local_num_chars;
    unsigned int size = fb_get_char_size(text_config.font_size);
    return (gpu_size){size * num_chars, (size + 2) * num_lines };
}

gpu_point calculate_label_pos(text_ui_config text_config, common_ui_config common_config){
    gpu_size s = calculate_label_size(text_config);
    gpu_point point = common_config.point;
    switch (common_config.horizontal_align)
    {
    case Trailing:
        point.x = (s.width>=common_config.size.width) ? common_config.point.x : (common_config.point.x + (common_config.size.width - s.width));
        break;
    case HorizontalCenter:
        point.x = (s.width>=common_config.size.width) ? common_config.point.x : (common_config.point.x + ((common_config.size.width - s.width)/2));
        break;
    default:
        break;
    }

    switch (common_config.vertical_align)
    {
    case Bottom:
        point.y = (s.height>=common_config.size.height) ? common_config.point.y : (common_config.point.y + (common_config.size.height - s.height));
        break;
    case VerticalCenter:
        point.y = (s.height>=common_config.size.height) ? common_config.point.y : (common_config.point.y + ((common_config.size.height - s.height)/2));
        break;
    default:
        break;
    }

    return point;
}

common_ui_config label(draw_ctx *ctx, text_ui_config text_config, common_ui_config common_config){
    if (!text_config.text || text_config.font_size==0) return common_config;
    gpu_point p = calculate_label_pos(text_config, common_config);
    fb_draw_string(ctx, text_config.text, p.x, p.y, text_config.font_size, common_config.foreground_color);
    return common_config;
}

common_ui_config textbox(draw_ctx *ctx, text_ui_config text_config, common_ui_config common_config){
    common_ui_config inner = rectangle(ctx, (rect_ui_config){0,0}, common_config);
    label(ctx, text_config, inner);
    return inner;
}

common_ui_config rectangle(draw_ctx *ctx, rect_ui_config rect_config, common_ui_config common_config){
    uint32_t bx = common_config.point.x, by = common_config.point.y, bw = common_config.size.width, bh = common_config.size.height, b = rect_config.border_size;
    if (rect_config.border_size > 0 && rect_config.border_size <= common_config.size.width && rect_config.border_size <= common_config.size.height){
        fb_fill_rect(ctx, bx, by, b, bh, rect_config.border_color);
        fb_fill_rect(ctx, bx + bw - b, by, b, bh, rect_config.border_color);
        fb_fill_rect(ctx, bx, by, bw, b, rect_config.border_color);
        fb_fill_rect(ctx, bx, by + bh - b, bw, b, rect_config.border_color);
    }
    
    uint32_t inner_x = bx + b;
    uint32_t inner_y = by + b;
    uint32_t twice_b = b << 1;
    uint32_t inner_w = (bw > twice_b) ? (bw - twice_b) : 0;
    uint32_t inner_h = (bh > twice_b) ? (bh - twice_b) : 0;
    if (inner_w && inner_h) fb_fill_rect(ctx, inner_x, inner_y, inner_w, inner_h, common_config.background_color);
    return (common_ui_config){
        .point = {inner_x, inner_y},
        .size = {inner_w, inner_h},
        .background_color = common_config.background_color,
        .foreground_color = common_config.foreground_color,
        .horizontal_align = common_config.horizontal_align,
        .vertical_align = common_config.vertical_align
    };
}
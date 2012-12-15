/**
 * vimp - a webkit based vim like browser.
 *
 * Copyright (C) 2012 Daniel Carl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include "main.h"
#include "dom.h"

static gboolean dom_auto_insert(Element* element);
static gboolean dom_editable_focus_cb(Element* element, Event* event);
static gboolean dom_is_editable(Element* element);
static WebKitDOMElement* dom_get_active_element(Document* doc);


void dom_check_auto_insert(void)
{
    Document* doc   = webkit_web_view_get_dom_document(vp.gui.webview);
    Element* active = dom_get_active_element(doc);
    if (!dom_auto_insert(active)) {
        HtmlElement* element = webkit_dom_document_get_body(doc);
        if (!element) {
            element = WEBKIT_DOM_HTML_ELEMENT(webkit_dom_document_get_document_element(doc));
        }
        webkit_dom_event_target_add_event_listener(
            WEBKIT_DOM_EVENT_TARGET(element), "focus", G_CALLBACK(dom_editable_focus_cb), true, NULL
        );
    }
}

void dom_element_set_style(Element* element, const gchar* format, ...)
{
    va_list args;
    va_start(args, format);
    gchar* value = g_strdup_vprintf(format, args);
    CssDeclaration* css = webkit_dom_element_get_style(element);
    if (css) {
        webkit_dom_css_style_declaration_set_css_text(css, value, NULL);
    }
    g_free(value);
}

void dom_element_style_set_property(Element* element, const gchar* property, const gchar* style)
{
    CssDeclaration* css = webkit_dom_element_get_style(element);
    if (css) {
        webkit_dom_css_style_declaration_set_property(css, property, style, "", NULL);
    }
}

gboolean dom_element_is_visible(Window* win, Element* element)
{
    if (webkit_dom_html_element_get_hidden(WEBKIT_DOM_HTML_ELEMENT(element))) {
        return FALSE;
    }

    CssDeclaration* style = webkit_dom_dom_window_get_computed_style(win, element, "");
    if (style_compare_property(style, "display", "none")) {
        return FALSE;
    }
    if (style_compare_property(style, "visibility", "hidde")) {
        return FALSE;
    }

    return TRUE;
}

DomBoundingRect dom_elemen_get_bounding_rect(Element* element)
{
    DomBoundingRect rect;
    rect.left = 0;
    rect.top  = 0;
    for (Element* e = element; e; e = webkit_dom_element_get_offset_parent(e)) {
        rect.left += webkit_dom_element_get_offset_left(e);
        rect.top  += webkit_dom_element_get_offset_top(e);
    }
    rect.right  = rect.left + webkit_dom_element_get_offset_width(element);
    rect.bottom = rect.top + webkit_dom_element_get_offset_height(element);

    return rect;
}

static gboolean dom_auto_insert(Element* element)
{
    if (dom_is_editable(element)) {
        vp_set_mode(VP_MODE_INSERT, FALSE);
        return TRUE;
    }
    return FALSE;
}

static gboolean dom_editable_focus_cb(Element* element, Event* event)
{
    webkit_dom_event_target_remove_event_listener(
        WEBKIT_DOM_EVENT_TARGET(element), "focus", G_CALLBACK(dom_editable_focus_cb), true
    );
    if (GET_CLEAN_MODE() != VP_MODE_INSERT) {
        EventTarget* target = webkit_dom_event_get_target(event);
        dom_auto_insert((void*)target);
    }
    return FALSE;
}

/**
 * Indicates if the given dom element is an editable element like text input,
 * password or textarea.
 */
static gboolean dom_is_editable(WebKitDOMElement* element)
{
    if (!element) {
        return FALSE;
    }

    gchar* tagname = webkit_dom_node_get_node_name(WEBKIT_DOM_NODE(element));
    if (!g_strcmp0(tagname, "TEXTAREA")) {
        return TRUE;
    }
    if (!g_strcmp0(tagname, "INPUT")) {
        gchar *type = webkit_dom_element_get_attribute((void*)element, "type");
        if (!g_strcmp0(type, "text") || !g_strcmp0(type, "password")) {
            return TRUE;
        }
    }
    return FALSE;
}

static Element* dom_get_active_element(Document* doc)
{
    Document* d     = NULL;
    Element* active = webkit_dom_html_document_get_active_element((void*)doc);
    gchar* tagname  = webkit_dom_element_get_tag_name(active);

    if (!g_strcmp0(tagname, "FRAME")) {
        d = webkit_dom_html_frame_element_get_content_document(WEBKIT_DOM_HTML_FRAME_ELEMENT(active));
        return dom_get_active_element(d);
    }
    if (!g_strcmp0(tagname, "IFRAME")) {
        d = webkit_dom_html_iframe_element_get_content_document(WEBKIT_DOM_HTML_IFRAME_ELEMENT(active));
        return dom_get_active_element(d);
    }
    return active;
}
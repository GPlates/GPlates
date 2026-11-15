{#- Sphinx's default class template, except for the class attributes that Boost.Python leaves
    undocumented ('class_attributes', from 'autosummary_context' in 'conf.py.in', which says why).
    The template writes their summary table, in the form autosummary gives its own, and their
    entries. (A class with other attributes too would get two tables under 'Attributes', but no
    class has both.) -#}
{{ fullname | escape | underline}}

.. currentmodule:: {{ module }}

.. autoclass:: {{ objname }}

   {% block methods %}
   .. automethod:: __init__

   {% if methods %}
   .. rubric:: {{ _('Methods') }}

   .. autosummary::
   {% for item in methods %}
      ~{{ name }}.{{ item }}
   {%- endfor %}
   {% endif %}
   {% endblock %}

   {% block attributes %}
   {%- set static_attributes = class_attributes.get(objname, []) %}
   {%- set other_attributes = attributes | reject('in', static_attributes | map(attribute='name') | list) | list %}
   {% if attributes %}
   .. rubric:: {{ _('Attributes') }}
   {% endif %}
   {% if other_attributes %}
   .. autosummary::
   {% for item in other_attributes %}
      ~{{ name }}.{{ item }}
   {%- endfor %}
   {% endif %}
   {% if static_attributes %}
   .. list-table::
      :class: autosummary longtable
      :widths: auto
   {% for attribute in static_attributes %}
      * - :py:obj:`{{ attribute.name }} <{{ fullname }}.{{ attribute.name }}>`
        - {{ attribute.text }}
   {%- endfor %}
   {% for attribute in static_attributes %}
   .. py:attribute:: {{ attribute.name }}
      :type: {{ attribute.type }}
   {%- if attribute.value %}
      :value: {{ attribute.value }}
   {%- endif %}

      {{ attribute.text }}
   {% endfor %}
   {%- endif %}
   {% endblock %}
